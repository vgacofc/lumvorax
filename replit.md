# Repl-Nix-Workspace — Quantum Research Project

## Overview
A Python-based scientific research environment focused on quantum physics and high-temperature superconductor (HTS) research. The project includes:
- Hubbard model / HTS simulation kernels (C source in `src/advanced_calculations/quantum_problem_hubbard_hts/`)
- Vesuvius challenge / competition kernels (`nx47_vesu_kernel_*.py`)
- ARC challenge research (`nx47-arc-kernel.py`)
- Kaggle submission tooling (`deploy_to_kaggle.py`, `kernel_to_push.py`)
- Scientific report generators (`generate_scientific_report.py`, `generate_performance_report.py`, etc.)
- LumVorax integration framework

## Architecture
- **Language**: Python 3.12
- **Package manager**: uv (with pyproject.toml)
- **Key dependencies**: numpy 1.26.4, scipy, torch (CPU), pandas, matplotlib, scikit-image, pyarrow, psycopg2-binary, kaggle, pillow, aiohttp
- **External DB**: Supabase PostgreSQL (credentials in environment variables)
- **nix packages**: arrow-cpp, cairo, clang, ffmpeg-full, gcc, ghostscript, gtk3, kaggle, libGL, and more (see replit.nix section in .replit)

## Important: libstdc++ Fix
Torch requires `libstdc++.so.6` to be preloaded before import. `main.py` handles this automatically by loading from the nix store path:
`/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6`

Any script that imports torch must either:
1. Import and run `main.py` first, OR
2. Preload the library manually via `ctypes.CDLL(path)` before `import torch`

## Workflows
- **Run Python**: Runs `python main.py` (entry point / smoke test)
- **Quantum Research Cycle C37**: Runs the full C-based HTS research cycle via `bash run_research_cycle.sh`

## Environment Variables (set in .replit)
- `KAGGLE_USERNAME`, `KAGGLE_API_TOKEN`, `KAGGLE_CONFIG_DIR`
- `SUPABASE_URL`, `SUPABASE_DB_*`, `SUPABASE_ANON_KEY`, `SUPABASE_SERVICE_ROLE_KEY`
- `ARISTOTLE_API_KEY`
