import os
import random
import sys
import threading
import time
from pathlib import Path
from typing import Any

from fastapi import FastAPI, HTTPException, UploadFile, File
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, Response
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

from src.acceptance import LAHCAcceptance, SimulatedAnnealingAcceptance
from src.construct import build_initial
from src.evaluator_ref import evaluate_cost_components, resolve_occurrences
from src.html_report.evaluation import build_constraint_scores, violation_event_refs
from src.html_report.grid import DayColumn, TimetableCell, build_days, build_resource_grid
from src.lahc import run_lahc
from src.model import Instance, Solution, SolutionGroup
from src.parser import parse_archive
from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector
from src.serializer import dict_to_instance, instance_to_dict
from src.xml_writer import (
    extract_instance_archive,
    render_archive_with_solution_groups,
    render_instance_xml,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ARCHIVE_PATH = REPO_ROOT / "data" / "xhstt2014" / "XHSTT-2014.xml"
CUSTOM_INSTANCES_DIR = REPO_ROOT / "data" / "custom_instances"
CUSTOM_INSTANCES_DIR.mkdir(parents=True, exist_ok=True)

app = FastAPI(title="School Timetabling Problem Solver API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


class StopRequestedException(Exception):
    pass


class SolverState:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.status: str = "idle"  # idle, running, completed, stopped, error
        self.instance_id: str | None = None
        self.iteration: int = 0
        self.total_iterations: int = 0
        self.infeasibility: int = 0
        self.objective: int = 0
        self.best_cost: int = 0
        self.rate: float = 0.0
        self.elapsed: float = 0.0
        self.error: str | None = None
        self.history: list[dict[str, Any]] = []
        self.stop_event = threading.Event()
        self.best_solution: Solution | None = None
        self.best_instance: Instance | None = None
        self.result: dict[str, Any] | None = None
        self.thread: threading.Thread | None = None

    def reset(self, instance_id: str, total_iterations: int) -> None:
        with self.lock:
            self.status = "running"
            self.instance_id = instance_id
            self.iteration = 0
            self.total_iterations = total_iterations
            self.infeasibility = 0
            self.objective = 0
            self.best_cost = 0
            self.rate = 0.0
            self.elapsed = 0.0
            self.error = None
            self.history = []
            self.stop_event.clear()
            self.best_solution = None
            self.best_instance = None
            self.result = None


solver_state = SolverState()


def get_all_instances() -> dict[str, Instance]:
    instances: dict[str, Instance] = {}
    if DEFAULT_ARCHIVE_PATH.exists():
        text = DEFAULT_ARCHIVE_PATH.read_text(encoding="utf-8-sig")
        for inst in parse_archive(text):
            instances[inst.id] = inst

    for file_path in CUSTOM_INSTANCES_DIR.glob("*.xml"):
        try:
            text = file_path.read_text(encoding="utf-8-sig")
            for inst in parse_archive(text):
                instances[inst.id] = inst
        except Exception:
            continue
    return instances


def get_instance_by_id(instance_id: str) -> Instance:
    instances = get_all_instances()
    if instance_id not in instances:
        raise HTTPException(status_code=404, detail=f"Instancja {instance_id} nie została znaleziona.")
    return instances[instance_id]


def build_timetable_payload(instance: Instance, solution: Solution) -> dict[str, Any]:
    occurrences = resolve_occurrences(instance, solution)
    days = build_days(instance)
    if not days and instance.time_groups:
        days = [
            DayColumn(
                id=g.id,
                name=g.name,
                periods=[t for t in instance.times if g.id in t.group_refs],
            )
            for g in instance.time_groups
        ]
    elif not days:
        days = [
            DayColumn(
                id="all",
                name="Wszystkie godziny",
                periods=list(instance.times),
            )
        ]

    days_data = [
        {
            "id": d.id,
            "name": d.name,
            "periods": [{"id": p.id, "name": p.name} for p in d.periods],
        }
        for d in days
    ]

    scores = build_constraint_scores(instance, occurrences)
    constraints_by_id = {c.id: c for c in instance.constraints}
    violation_map = {
        s.id: violation_event_refs(instance, occurrences, constraints_by_id[s.id])
        for s in scores
        if s.cost > 0 and s.id in constraints_by_id
    }

    grids_data: dict[str, list[dict[str, Any]]] = {}
    for r in instance.resources:
        grid = build_resource_grid(instance, occurrences, r.id)
        cells = []
        for (day_id, period_idx), entries in grid.items():
            for entry in entries:
                cells.append(
                    {
                        "day_id": day_id,
                        "period_idx": period_idx,
                        "event_ref": entry.event_ref,
                        "event_name": entry.event_name,
                        "duration": entry.duration,
                        "other_resources": entry.other_resources,
                        "is_clash": len(entries) > 1,
                    }
                )
        grids_data[r.id] = cells

    infeasibility, objective = evaluate_cost_components(instance, solution)
    scores_data = [
        {
            "id": s.id,
            "name": s.name,
            "required": s.required,
            "weight": s.weight,
            "cost": s.cost,
        }
        for s in scores
    ]

    return {
        "days": days_data,
        "grids": grids_data,
        "scores": scores_data,
        "violation_map": violation_map,
        "infeasibility": infeasibility,
        "objective": objective,
    }


@app.get("/api/instances")
def list_instances() -> list[dict[str, Any]]:
    instances = get_all_instances()
    result = []
    custom_ids = {
        inst.id
        for f in CUSTOM_INSTANCES_DIR.glob("*.xml")
        for inst in parse_archive(f.read_text(encoding="utf-8-sig"))
    }
    for inst in sorted(instances.values(), key=lambda i: i.id):
        result.append(
            {
                "id": inst.id,
                "name": inst.name or inst.id,
                "events_count": len(inst.events),
                "times_count": len(inst.times),
                "resources_count": len(inst.resources),
                "constraints_count": len(inst.constraints),
                "is_custom": inst.id in custom_ids,
            }
        )
    return result


@app.get("/api/instances/{instance_id}")
def get_instance_details(instance_id: str) -> dict[str, Any]:
    inst = get_instance_by_id(instance_id)
    return instance_to_dict(inst)


@app.post("/api/instances")
def save_instance(payload: dict[str, Any]) -> dict[str, Any]:
    try:
        inst = dict_to_instance(payload)
        xml_content = render_instance_xml(inst)
        file_path = CUSTOM_INSTANCES_DIR / f"{inst.id}.xml"
        file_path.write_text(xml_content, encoding="utf-8")
        return {"status": "ok", "id": inst.id, "message": "Instancja zapisana pomyślnie."}
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Błąd zapisu instancji: {e}") from e


@app.post("/api/instances/upload")
async def upload_instance_xml(file: UploadFile = File(...)) -> dict[str, Any]:
    try:
        content = await file.read()
        text = content.decode("utf-8-sig", errors="replace")
        parsed = parse_archive(text)
        if not parsed:
            raise ValueError("Plik nie zawiera żadnej poprawnej instancji XHSTT.")
        saved_ids = []
        for inst in parsed:
            xml_content = render_instance_xml(inst)
            target = CUSTOM_INSTANCES_DIR / f"{inst.id}.xml"
            target.write_text(xml_content, encoding="utf-8")
            saved_ids.append(inst.id)
        return {"status": "ok", "imported_ids": saved_ids}
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Błąd importu pliku: {e}") from e


@app.get("/api/instances/{instance_id}/export")
def export_instance_xml(instance_id: str) -> Response:
    inst = get_instance_by_id(instance_id)
    xml_str = render_instance_xml(inst)
    return Response(
        content=xml_str,
        media_type="application/xml",
        headers={"Content-Disposition": f'attachment; filename="{inst.id}.xml"'},
    )


class SolveRequest(BaseModel):
    instance_id: str
    iterations: int = 20_000
    selector: str = "ucb"  # random, epsilon-greedy, ucb
    acceptance: str = "lahc"  # lahc, sa
    seed: int = 0
    time_limit: float | None = None
    history_length: int = 30
    sa_t_start: float = 10_000.0
    sa_t_end: float = 1.0
    sa_cooling: float = 0.9999


def _run_solver_thread(req: SolveRequest, instance: Instance) -> None:
    rng = random.Random(req.seed)
    initial = build_initial(instance, rng)
    infeasibility_0, objective_0 = evaluate_cost_components(instance, initial)

    with solver_state.lock:
        solver_state.infeasibility = infeasibility_0
        solver_state.objective = objective_0
        solver_state.best_cost = infeasibility_0 * 1_000_000 + objective_0
        solver_state.history.append(
            {
                "iteration": 0,
                "infeasibility": infeasibility_0,
                "objective": objective_0,
                "elapsed": 0.0,
            }
        )

    t_start = time.time()
    last_record_iter = 0

    def on_progress(iteration: int, best_cost: int) -> None:
        nonlocal last_record_iter
        if solver_state.stop_event.is_set():
            raise StopRequestedException("Zatrzymano na żądanie użytkownika.")

        elapsed = time.time() - t_start
        rate = iteration / elapsed if elapsed > 0 else 0.0
        infeasibility = best_cost // 1_000_000
        objective = best_cost % 1_000_000

        with solver_state.lock:
            solver_state.iteration = iteration
            solver_state.best_cost = best_cost
            solver_state.infeasibility = infeasibility
            solver_state.objective = objective
            solver_state.rate = round(rate, 1)
            solver_state.elapsed = round(elapsed, 1)

            if iteration - last_record_iter >= max(50, req.iterations // 200) or iteration == req.iterations:
                last_record_iter = iteration
                solver_state.history.append(
                    {
                        "iteration": iteration,
                        "infeasibility": infeasibility,
                        "objective": objective,
                        "elapsed": round(elapsed, 2),
                    }
                )

    # Selector
    if req.selector == "ucb":
        selector = UCBSelector()
    elif req.selector == "epsilon-greedy":
        selector = EpsilonGreedySelector()
    else:
        selector = RandomSelector()

    # Acceptance
    if req.acceptance == "sa":
        acceptance = SimulatedAnnealingAcceptance(
            t_start=req.sa_t_start,
            t_end=req.sa_t_end,
            cooling_rate=req.sa_cooling,
        )
    else:
        acceptance = LAHCAcceptance(history_length=req.history_length)

    try:
        best_solution, _ = run_lahc(
            instance=instance,
            initial=initial,
            rng=rng,
            history_length=req.history_length,
            selector=selector,
            max_iterations=req.iterations,
            max_seconds=req.time_limit,
            on_progress=on_progress,
            progress_every=max(1, req.iterations // 100),
            progress_seconds=0.5,
            evaluation="incremental",
            acceptance=acceptance,
        )
        final_infeasibility, final_objective = evaluate_cost_components(instance, best_solution)
        with solver_state.lock:
            solver_state.status = "completed"
            solver_state.best_solution = best_solution
            solver_state.best_instance = instance
            solver_state.infeasibility = final_infeasibility
            solver_state.objective = final_objective
            solver_state.result = build_timetable_payload(instance, best_solution)
    except StopRequestedException:
        with solver_state.lock:
            solver_state.status = "stopped"
            if solver_state.best_solution is not None:
                solver_state.result = build_timetable_payload(instance, solver_state.best_solution)
    except Exception as exc:
        with solver_state.lock:
            solver_state.status = "error"
            solver_state.error = str(exc)


@app.post("/api/solver/start")
def start_solver(req: SolveRequest) -> dict[str, Any]:
    with solver_state.lock:
        if solver_state.status == "running":
            raise HTTPException(status_code=400, detail="Solver już pracuje w tle.")
    instance = get_instance_by_id(req.instance_id)
    solver_state.reset(req.instance_id, req.iterations)
    thread = threading.Thread(target=_run_solver_thread, args=(req, instance), daemon=True)
    solver_state.thread = thread
    thread.start()
    return {"status": "started", "instance_id": req.instance_id}


@app.post("/api/solver/stop")
def stop_solver() -> dict[str, Any]:
    solver_state.stop_event.set()
    return {"status": "stopping"}


@app.get("/api/solver/status")
def get_solver_status() -> dict[str, Any]:
    with solver_state.lock:
        return {
            "status": solver_state.status,
            "instance_id": solver_state.instance_id,
            "iteration": solver_state.iteration,
            "total_iterations": solver_state.total_iterations,
            "infeasibility": solver_state.infeasibility,
            "objective": solver_state.objective,
            "best_cost": solver_state.best_cost,
            "rate": solver_state.rate,
            "elapsed": solver_state.elapsed,
            "error": solver_state.error,
            "history": solver_state.history[-150:],
            "has_result": solver_state.result is not None,
        }


@app.get("/api/solver/result")
def get_solver_result() -> dict[str, Any]:
    with solver_state.lock:
        if solver_state.result is None:
            raise HTTPException(status_code=404, detail="Brak dostępnego wyniku rozwiązania.")
        return solver_state.result


@app.get("/api/solver/download")
def download_solution_xml() -> Response:
    with solver_state.lock:
        if solver_state.best_solution is None or solver_state.best_instance is None:
            raise HTTPException(status_code=404, detail="Brak rozwiązania do pobrania.")
        inst = solver_state.best_instance
        sol = solver_state.best_solution

    single_instance_xml = render_instance_xml(inst)
    group = SolutionGroup(id=f"LAHC_{inst.id}", solutions=[sol])
    full_xml = render_archive_with_solution_groups(single_instance_xml, [group])
    return Response(
        content=full_xml,
        media_type="application/xml",
        headers={"Content-Disposition": f'attachment; filename="{inst.id}_solution.xml"'},
    )


# Serve frontend static files if built
FRONTEND_DIST = REPO_ROOT / "frontend" / "dist"
if FRONTEND_DIST.exists():
    app.mount("/", StaticFiles(directory=str(FRONTEND_DIST), html=True), name="frontend")
