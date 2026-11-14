"""LLM REST client with persistent disk cache.

Supports two backends selected via LLM_BACKEND env var:
  "ollama"  (default) — Ollama API at OLLAMA_HOST
  "openai"            — OpenAI-compatible API at OPENAI_BASE_URL (vLLM, etc.)

Configuration via environment variables:
  LLM_BACKEND      — "ollama" or "openai", default "ollama"
  OLLAMA_HOST      — Ollama base URL, default http://localhost:11434
  OLLAMA_MODEL     — Ollama model tag, default qwen2.5-coder:32b
  OPENAI_BASE_URL  — OpenAI-compatible base URL, default http://localhost:8000/v1
  OPENAI_MODEL     — model name for OpenAI-compatible backend,
                     default deepseek-ai/DeepSeek-V4.1-Flash-1017
  OPENAI_API_KEY   — API key (optional, vLLM does not require one)

Every call (hit or miss) is appended to generated/llm_cache.jsonl so that
the full prompt->response log is preserved for the thesis chapter.  Cache
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

LLM_BACKEND: str = os.environ.get("LLM_BACKEND", "ollama")

OLLAMA_HOST: str = os.environ.get("OLLAMA_HOST", "http://localhost:11434")
OLLAMA_MODEL: str = os.environ.get("OLLAMA_MODEL", "qwen2.5-coder:32b")

OPENAI_BASE_URL: str = os.environ.get("OPENAI_BASE_URL", "http://localhost:8000/v1")
OPENAI_MODEL: str = os.environ.get(
    "OPENAI_MODEL", "deepseek-ai/DeepSeek-V4.1-Flash-1017"
)
OPENAI_API_KEY: str = os.environ.get("OPENAI_API_KEY", "EMPTY")

# Default model per backend — used when caller passes the module-level default.
DEFAULT_MODEL: str = OPENAI_MODEL if LLM_BACKEND == "openai" else OLLAMA_MODEL

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
        "backend": LLM_BACKEND,
    }
    with _CACHE_FILE.open("a", encoding="utf-8") as fh:
        fh.write(json.dumps(entry, ensure_ascii=False) + "\n")


def _call_ollama(
    messages: list[dict[str, str]],
    model: str,
    temperature: float,
    max_tokens: int,
) -> tuple[str, int, int]:
    """Returns (content, prompt_tokens, completion_tokens)."""
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
    data = raw.json()
    return (
        data["message"]["content"],
        data.get("prompt_eval_count", 0),
        data.get("eval_count", 0),
    )


def _call_openai(
    messages: list[dict[str, str]],
    model: str,
    temperature: float,
    max_tokens: int,
) -> tuple[str, int, int]:
    """Returns (content, prompt_tokens, completion_tokens).
    Compatible with vLLM, OpenAI, and any OpenAI-compatible endpoint."""
    headers = {"Authorization": f"Bearer {OPENAI_API_KEY}"}
    with httpx.Client(timeout=_HTTP_TIMEOUT, headers=headers) as client:
        raw = client.post(
            f"{OPENAI_BASE_URL}/chat/completions",
            json={
                "model": model,
                "messages": messages,
                "temperature": temperature,
                "max_tokens": max_tokens,
            },
        )
        raw.raise_for_status()
    data = raw.json()
    usage = data.get("usage", {})
    return (
        data["choices"][0]["message"]["content"],
        usage.get("prompt_tokens", 0),
        usage.get("completion_tokens", 0),
    )


def call(
    messages: list[dict[str, str]],
    model: str = DEFAULT_MODEL,
    temperature: float = 0.8,
    max_tokens: int = 1024,
    use_cache: bool = True,
) -> LLMResponse:
    """Send a chat request to the configured backend; return cached response when available.

    Args:
        messages:    OpenAI-style list of {role, content} dicts.
        model:       Model tag/name (overrides DEFAULT_MODEL).
        temperature: Sampling temperature — 0.8 gives diverse candidates.
        max_tokens:  Maximum tokens to generate.
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
    if LLM_BACKEND == "openai":
        content, prompt_tokens, completion_tokens = _call_openai(
            messages, model, temperature, max_tokens
        )
    else:
        content, prompt_tokens, completion_tokens = _call_ollama(
            messages, model, temperature, max_tokens
        )
    elapsed = time.monotonic() - t0

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
