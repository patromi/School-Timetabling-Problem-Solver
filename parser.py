import xml.etree.ElementTree as ET
import pandas as pd
from pathlib import Path


def parse_instance(xml_path: str | Path) -> dict[str, pd.DataFrame]:
    """Parse an ITC-2019 XML timetabling instance into a dict of DataFrames."""
    tree = ET.parse(xml_path)
    root = tree.getroot()

    meta = {
        "name": root.attrib["name"],
        "nrDays": int(root.attrib["nrDays"]),
        "nrDays": int(root.attrib["nrDays"]),
        "nrDays": int(root.attrib["nrDays"]),
        "slotsPerDay": int(root.attrib["slotsPerDay"]),
        "nrWeeks": int(root.attrib["nrWeeks"]),
    }
    opt = root.find("optimization").attrib
    meta.update({f"w_{k}": int(v) for k, v in opt.items()})
    df_meta = pd.DataFrame([meta])

    # --- Rooms ---
    rooms_rows, travel_rows, unavail_rows = [], [], []
    for room in root.findall("rooms/room"):
        rid = int(room.attrib["id"])
        rooms_rows.append({"room_id": rid, "capacity": int(room.attrib["capacity"])})
        for t in room.findall("travel"):
            travel_rows.append({
                "from_room": rid,
                "to_room": int(t.attrib["room"]),
                "travel_time": int(t.attrib["value"]),
            })
        for u in room.findall("unavailable"):
            unavail_rows.append({
                "room_id": rid,
                "days": u.attrib["days"],
                "start": int(u.attrib["start"]),
                "length": int(u.attrib["length"]),
                "weeks": u.attrib["weeks"],
            })

    df_rooms = pd.DataFrame(rooms_rows).astype({"room_id": int, "capacity": int})
    df_travel = pd.DataFrame(travel_rows) if travel_rows else pd.DataFrame(
        columns=["from_room", "to_room", "travel_time"]
    )
    df_unavail_rooms = pd.DataFrame(unavail_rows) if unavail_rows else pd.DataFrame(
        columns=["room_id", "days", "start", "length", "weeks"]
    )

    # --- Courses / Configs / Subparts / Classes ---
    class_rows, class_room_rows, class_time_rows = [], [], []
    for course in root.findall("courses/course"):
        cid = int(course.attrib["id"])
        for config in course.findall("config"):
            cfg_id = int(config.attrib["id"])
            for subpart in config.findall("subpart"):
                sp_id = int(subpart.attrib["id"])
                for cls in subpart.findall("class"):
                    cl_id = int(cls.attrib["id"])
                    class_rows.append({
                        "class_id": cl_id,
                        "course_id": cid,
                        "config_id": cfg_id,
                        "subpart_id": sp_id,
                        "limit": int(cls.attrib["limit"]),
                    })
                    for r in cls.findall("room"):
                        class_room_rows.append({
                            "class_id": cl_id,
                            "room_id": int(r.attrib["id"]),
                            "penalty": int(r.attrib["penalty"]),
                        })
                    for t in cls.findall("time"):
                        class_time_rows.append({
                            "class_id": cl_id,
                            "days": t.attrib["days"],
                            "start": int(t.attrib["start"]),
                            "length": int(t.attrib["length"]),
                            "weeks": t.attrib["weeks"],
                            "penalty": int(t.attrib["penalty"]),
                        })

    df_classes = pd.DataFrame(class_rows)
    df_class_rooms = pd.DataFrame(class_room_rows) if class_room_rows else pd.DataFrame(
        columns=["class_id", "room_id", "penalty"]
    )
    df_class_times = pd.DataFrame(class_time_rows) if class_time_rows else pd.DataFrame(
        columns=["class_id", "days", "start", "length", "weeks", "penalty"]
    )

    # --- Distributions ---
    dist_rows = []
    for dist in root.findall("distributions/distribution"):
        required = dist.attrib.get("required", "false").lower() == "true"
        penalty = int(dist.attrib["penalty"]) if "penalty" in dist.attrib else None
        class_ids = [int(c.attrib["id"]) for c in dist.findall("class")]
        for cl_id in class_ids:
            dist_rows.append({
                "type": dist.attrib["type"],
                "required": required,
                "penalty": penalty,
                "class_id": cl_id,
            })

    df_distributions = pd.DataFrame(dist_rows) if dist_rows else pd.DataFrame(
        columns=["type", "required", "penalty", "class_id"]
    )

    # --- Students ---
    student_rows = []
    for student in root.findall("students/student"):
        sid = int(student.attrib["id"])
        for course in student.findall("course"):
            student_rows.append({
                "student_id": sid,
                "course_id": int(course.attrib["id"]),
            })

    df_students = pd.DataFrame(student_rows) if student_rows else pd.DataFrame(
        columns=["student_id", "course_id"]
    )

    return {
        "meta": df_meta,
        "rooms": df_rooms,
        "room_travel": df_travel,
        "room_unavailable": df_unavail_rooms,
        "classes": df_classes,
        "class_rooms": df_class_rooms,
        "class_times": df_class_times,
        "distributions": df_distributions,
        "students": df_students,
    }


if __name__ == "__main__":
    import sys

    path = sys.argv[1] if len(sys.argv) > 1 else "instances/pu-c8-spr07.xml"
    data = parse_instance(path)

    for name, df in data.items():
        print(f"\n{'='*60}")
        print(f"  {name}  ({len(df)} rows)")
        print("="*60)
        print(df.head(10).to_string(index=False))
