"""LLM-based heuristic generator for the school timetabling hyper-heuristic.

Subpackage structure:
  client.py    — Ollama REST client with disk-based response cache
  prompt.py    — diagnostic prompt builder (variant A, B, RAG)
  kb.py        — rule-based knowledge base retrieval (kb/ directory)
  sandbox.py   — AST static analysis + subprocess isolation
  validator.py — 4-step validation pipeline
  pool.py      — orchestrator: stagnation trigger + pool synchronisation
"""
