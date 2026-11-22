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
  LLM_MAX_TOKENS   — default completion budget, default 1024; reasoning
                     models (DeepSeek-R1) need far more, e.g. 16384, since
                     the hidden reasoning counts against it
  LLM_HTTP_TIMEOUT — HTTP timeout in seconds, default 300

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

from src.llm.filelock import append_line

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
# 5 min by default — large models can be slow on first token; a reasoning
# model under load from many parallel runs needs more (LLM_HTTP_TIMEOUT)
_HTTP_TIMEOUT = float(os.environ.get("LLM_HTTP_TIMEOUT", "300"))
DEFAULT_MAX_TOKENS: int = int(os.environ.get("LLM_MAX_TOKENS", "1024"))

# In-memory cache: loaded lazily on first use, updated on every write so
# repeated call()s within one process never re-read the file from disk.
_CACHE: dict[str, str] | None = None


@dataclass(frozen=True)
class LLMResponse:
    content: str
    model: str
    prompt_tokens: int
    completion_tokens: int
    elapsed_s: float
    cached: bool


def _cache_key(model: str, messages: list[dict[str, str]], sample_idx: int = 0) -> str:
    """Key of one cached response. `sample_idx` tells apart several samples
    drawn for the same prompt (the candidates of one LLM call); sample 0
    keeps the original key, so caches written before it still hit."""
    content: dict[str, object] = {"model": model, "messages": messages}
    if sample_idx:
        content["sample_idx"] = sample_idx
    payload = json.dumps(content, sort_keys=True)
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


def _get_cache() -> dict[str, str]:
    global _CACHE
    if _CACHE is None:
        _CACHE = _read_cache()
    return _CACHE


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
    append_line(_CACHE_FILE, json.dumps(entry, ensure_ascii=False))
    # Keep the in-memory cache in sync so the next call() doesn't re-read disk.
    if _CACHE is not None:
        _CACHE[key] = content


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
    choice = data["choices"][0]
    # with --reasoning-parser vLLM moves the reasoning to reasoning_content;
    # content is null when the token budget ran out before the answer
    content = _strip_reasoning(choice["message"].get("content") or "")
    if not content.strip():
        raise RuntimeError(
            f"empty answer (finish_reason={choice.get('finish_reason')}, "
            f"completion_tokens={usage.get('completion_tokens', 0)}); "
            "raise LLM_MAX_TOKENS"
        )
    return (
        content,
        usage.get("prompt_tokens", 0),
        usage.get("completion_tokens", 0),
    )


def _strip_reasoning(content: str) -> str:
    """Drop a <think>...</think> block left in the answer by a reasoning
    model served without a reasoning parser; its draft code would otherwise
    be merged with the final code by the fence extraction."""
    if "</think>" in content:
        return content.rsplit("</think>", 1)[1]
    return content


def call(
    messages: list[dict[str, str]],
    model: str = DEFAULT_MODEL,
    temperature: float = 0.8,
    max_tokens: int = DEFAULT_MAX_TOKENS,
    use_cache: bool = True,
    sample_idx: int = 0,
) -> LLMResponse:
    """Send a chat request to the configured backend; return cached response when available.

    Args:
        messages:    OpenAI-style list of {role, content} dicts.
        model:       Model tag/name (overrides DEFAULT_MODEL).
        temperature: Sampling temperature — 0.8 gives diverse candidates.
        max_tokens:  Maximum tokens to generate (LLM_MAX_TOKENS by default).
        use_cache:   When False, always calls the API (useful for testing).
        sample_idx:  Index of this sample among several drawn for the same
                     prompt. Without it every candidate after the first
                     would be a cache hit returning the first one's code.
    """
    key = _cache_key(model, messages, sample_idx)

    if use_cache:
        cache = _get_cache()
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
