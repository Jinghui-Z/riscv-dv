# ai_test_cases/mix

Place AI cases here only when a case intentionally spans multiple major
responsibility modules and no single `backend/`, `frontend/`, or `memblock/`
directory would describe it clearly.

Prefer a precise module subdirectory when one module dominates. Use this
top-level `mix/` bucket for real cross-major-module interactions such as
debug-trigger cases that combine fetch, traps, and memory-side effects.
