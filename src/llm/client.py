"""Ollama REST client with persistent disk cache.

Configuration via environment variables:
  OLLAMA_HOST  — base URL, default http://localhost:11434
  OLLAMA_MODEL — model tag, default qwen2.5-coder:32b

Every call (hit or miss) is appended to generated/llm_cache.jsonl so that
the full prompt→response log is preserved for the thesis chapter.  Cache
lookups key on sha256(model + serialised messages) — identical diagnostic
state in repeated runs returns the cached response without touching the GPU.
"""

import hashlib
import json
import os
import time
from dataclasses import dataclass
from pathlib import Path

import httpx

OLLAMA_HOST: str = os.environ.get("OLLAMA_HOST", "http://localhost:11434")
OLLAMA_MODEL: str = os.environ.get("OLLAMA_MODEL", "qwen2.5-coder:32b")

_CACHE_FILE = Path("generated/llm_cache.jsonl")
_HTTP_TIMEOUT = 300.0  # 5 min — large models can be slow on first token


@dataclass(frozen=True)
class LLMResponse:
    content: str
    model: str
    prompt_tokens: int
    completion_tokens: int
    elapsed_s: float
    cached: bool


def _cache_key(model: str, messages: list[dict[str, str]]) -> str:
    payload = json.dumps({"model": model, "messages": messages}, sort_keys=True)
    return hashlib.sha256(payload.encode()).hexdigest()


def _read_cache() -> dict[str, str]:
    if not _CACHE_FILE.exists():
        return {}
    cache: dict[str, str] = {}
    with _CACHE_FILE.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            try:
                entry = json.loads(line)
                cache[entry["key"]] = entry["content"]
            except (json.JSONDecodeError, KeyError):
                pass
    return cache


def _append_cache(
    key: str,
    content: str,
    model: str,
    prompt_tokens: int,
    completion_tokens: int,
    elapsed_s: float,
    cached: bool,
    messages: list[dict[str, str]],
) -> None:
    _CACHE_FILE.parent.mkdir(parents=True, exist_ok=True)
    entry = {
        "key": key,
        "content": content,
        "model": model,
        "prompt_tokens": prompt_tokens,
        "completion_tokens": completion_tokens,
        "elapsed_s": elapsed_s,
        "cached": cached,
        "timestamp": time.time(),
        "messages": messages,
    }
    with _CACHE_FILE.open("a", encoding="utf-8") as fh:
        fh.write(json.dumps(entry, ensure_ascii=False) + "\n")


def call(
    messages: list[dict[str, str]],
    model: str = OLLAMA_MODEL,
    temperature: float = 0.8,
    max_tokens: int = 1024,
    use_cache: bool = True,
) -> LLMResponse:
    """Send a chat request to Ollama; return cached response when available.

    Args:
        messages:    OpenAI-style list of {role, content} dicts.
        model:       Ollama model tag (overrides OLLAMA_MODEL env var).
        temperature: Sampling temperature — 0.8 gives diverse candidates.
        max_tokens:  Maximum tokens to generate (num_predict in Ollama).
        use_cache:   When False, always calls the API (useful for testing).
    """
    key = _cache_key(model, messages)

    if use_cache:
        cache = _read_cache()
        if key in cache:
            return LLMResponse(
                content=cache[key],
                model=model,
                prompt_tokens=0,
                completion_tokens=0,
                elapsed_s=0.0,
                cached=True,
            )

    t0 = time.monotonic()
    with httpx.Client(timeout=_HTTP_TIMEOUT) as client:
        raw = client.post(
            f"{OLLAMA_HOST}/api/chat",
            json={
                "model": model,
                "messages": messages,
                "stream": False,
                "options": {
                    "temperature": temperature,
                    "num_predict": max_tokens,
                },
            },
        )
        raw.raise_for_status()

    elapsed = time.monotonic() - t0
    data = raw.json()
    content: str = data["message"]["content"]
    prompt_tokens: int = data.get("prompt_eval_count", 0)
    completion_tokens: int = data.get("eval_count", 0)

    resp = LLMResponse(
        content=content,
        model=model,
        prompt_tokens=prompt_tokens,
        completion_tokens=completion_tokens,
        elapsed_s=elapsed,
        cached=False,
    )
    _append_cache(
        key=key,
        content=content,
        model=model,
        prompt_tokens=prompt_tokens,
        completion_tokens=completion_tokens,
        elapsed_s=elapsed,
        cached=False,
        messages=messages,
    )
    return resp
