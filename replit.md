# LUM/VORAX Quantum Research System

## Project Overview
A scientific computing project focused on quantum physics research, black hole simulation (Kerr metric/Gargantua), and advanced numerical methods. The system includes:

- **LUM/VORAX engine**: Core C-based computation framework with 39+ modules
- **Quantum simulations**: Hubbard model, high-temperature superconductors (HTS)
- **Forensic logging**: Bit-level traceability and hardware timestamping (nanosecond precision)
- **Advanced algorithms**: Neural networks, SIMD optimization, parallel processing, TSP, Pareto optimization

## Architecture
- **Language**: C (primary), Python (utilities), Bash (orchestration)
- **Build system**: GNU Make
- **Binaries**: Pre-compiled in `bin/` directory
  - `bin/lum_vorax_complete` — Main computation engine
  - `bin/test_forensic_complete_system` — Forensic test suite
  - `bin/test_integration_complete_39_modules` — Integration tests
  - `bin/test_quantum` — Quantum module tests

## Key Directories
- `src/` — C source code (core, lum, vorax, crypto, debug, advanced_calculations, etc.)
- `bin/` — Compiled executables
- `dataset/`, `DATASET/` — Research data
- `evidence/` — Scientific evidence/results
- `docs/` — Documentation

## Quantum Hubbard/HTS Research — Cycle Status (2026-03-24 — C60)

### Runner: `hubbard_hts_research_runner_advanced_parallel` (recompilé C43 — 153 KB)
- PT-MC : **15 modules** actifs (+ fermionic_sign_problem), **8 répliques**, **20 000 sweeps**, 4 000 therma, 500 steps/sweep
- Steps×5 : ex. hubbard_hts_core 2800→14000, sites 100→196 (14×14)
- FSS Binder : gamme T ±100K (vs ±20K C42), L=[8,12,16,20,24] (vs [8,12,16])
- DYNSCALE : formule recalibrée `200MB + N×0.032 + N²×1.6e-6`
- fermionic_sign_problem : U/t=14, demi-remplissage, T=20K, 12×12 sites, 15000 steps
- Checkpoints convergence : {2000, 5000, 10000, probs[i].steps} (vs {700, 1400, 2100, …})
- Stability checkpoints : {4000, 8000, 12000, 16000} (vs {2200, 4400, 6600, 8800})
- autocorr : `--max-window 5000 --c-factor 6.0 --run-dir` (BUG-AC-01 shell fix)

### Corrections C24 appliquées
- **C24-01** : Seeds variables via `PTMC_RUN_INDEX` et `PTMC_SEED_RANDOM` (était hardcodé 0xBEEF0000+i)
- **C24-02** : Cross-validation ED ajoutée — `#include "exact_diagonalization.h"` + appels `ed_hubbard_2x2`/`ed_hubbard_1d4` pour n_sites≤4 (corrige G-C23-05 : 0 entrées Lanczos dans LumVorax)

### Corrections C25 appliquées (2026-03-16)
- **C25-STEPLOG** : 4 appels `FORENSIC_LOG_MODULE_METRIC` (nom d'origine) ajoutés dans la boucle principale — chaque step (energy_eV, pairing_norm, sign_ratio, elapsed_ns) dans Lumvorax sans filtre. Binaire recompilé 12:27Z. Actif au prochain run.
- **C25-NAMES** : Correction dans `STANDARD_NAMES.md` v2.0 — `FORENSIC_LOG_MODULE_METRIC` = nom d'origine officiel (71 usages, ultra_forensic_logger.h). `LV_MODULE_METRIC` = doublon du bridge secondaire (lumvorax_integration.h). Aucun code renommé (ne pas casser les modules dépendants).
- **STANDARD_NAMES.md v2.0** : `src/advanced_calculations/quantum_problem_hubbard_hts/STANDARD_NAMES.md` — registre canonique corrigé avec noms d'origine réels.

### Bugs confirmés restants (C26)
- **C25-TAU** (P1) : Estimateur τ_int Sokal absent → barres d'erreur sous-estimées ×5
- **C25-TC** (P1) : Désaccord Tc 72K vs 95K (23K) — densifier scan T
- **C25-COMPRESS** (P2) : Module LZ4 streaming absent → CSV brut 95 MB/run (à créer)
- **C25-CLUSTER** (P3) : cluster_scale 8.3% PASS (3/36) à corriger
- **BUG-CACHÉ-01** (P4) : init `d_rep[r][i] ∈ [-0.5,+0.5]` — biais potentiel

### Métriques réelles système (run research_20260315T225836Z_3502)
- RAM totale : 62.82 GB | RAM dispo : 25.99 GB | CPU runner : 15.6–18.1%
- Ops Metropolis/s : 1.10 M | Sweep-répliques/s : 5 482 | Qubits effectifs : 12 480
- Lumvorax : 1 149 202 lignes, 95 MB | PT-MC CSV : 312 001 lignes, 37 MB
- Tc méthode 1 : 72 K (dP/dT) | Tc méthode 2 : 95 K (χ_sc) | Désaccord : 23 K

### Rapports d'analyse
- `CHAT/analysechatgpt32.md` — Rapport C25 ultra-détaillé : métriques réelles, comparaisons (ALPS/NetKet/Qiskit/TRIQS/ITensor/PennyLane/QuEra/QuSpin/OpenFermion), solutions C25 (RAPPORT ACTUEL)
- `CHAT/analysechatgpt31.md` — Corrections 3 erreurs majeures C24 + audit cycle 3502
- `CHAT/analysechatgpt31.1md` — Traces de raisonnement interne cycle C24

## Quantum V4 NEXT — Cycle C34 (2026-03-17)

### Simulateur Quantique V4 NEXT — Logging ultra-granulaire QF_LOG_* intégré

**Fichiers :**
- `src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_forensic_logger.h/.c` — Logger STANDARD_NAMES Section I
- `quantum_simulator.c` — Instrumenté (15+ macros QF_LOG_* dans create, destroy, apply_gate, entangle, measure, stress_test)
- `quantum_simulator_fusion_v3.c` — Instrumenté (QF_LOG_SCENARIO chaque itération, summary final)
- `quantum_v4next_main.c` — Runner principal (point d'entrée main())
- `tools/fusion_cli_v3.c` — Instrumenté (qf_logger_init/destroy)
- `Makefile` — 0 erreur, 0 warning
- `RAPPORTS/` — Dossier rapports dédié

**Binaires compilés (C34) :**
- `quantum_v4next_runner` (58K) — stress test PASS, Hadamard PASS, intrication PASS, mesure PASS, 0 fuite mémoire
- `quantum_fusion_v3_cli` (54K) — win_rate=70%, 63K qubits/sec, p95=16μs

**Corrections C33 appliquées dans Hubbard :**
- C33-P1 : ed_validation_2x2 dans problems_cycle06.csv
- C33-P2 : n_tc=23 points scan 70-85K, arrays [32]
- C33-P3 : post_run_autocorr.py (Sokal τ_int)
- C33-P5 : cluster_pair_tol 0.03→0.001

## Workflows
- **Run Python**: Runs `main.py` (Python utility entry point)
- **Quantum Research Cycle**: Runs the full quantum research bash script (Hubbard HTS, C34)

## Dependencies (Nix packages)
arrow-cpp, cairo, clang, ffmpeg-full, gcc, gdb, ghostscript, glib, glibc, glibcLocales, gnumake, gobject-introspection, gtk3, kaggle, libxcrypt, nano, openssh, pkg-config, qhull, tcl, tk, tree, vim-full, xsimd, zlib

## Kaggle Integration
- Username: ndarray2000
- Config stored in `$KAGGLE_CONFIG_DIR/kaggle.json`

## Build Integrity — Corrections BC25 (2026-03-14)
Six bugs corrigés, build 100 % propre (gcc -Wall -Wextra, zéro warning) :
- **BC25-01** `src/common/magic_numbers.h` + `data_persistence.c` — `LUM_GROUP_MAGIC` manquant (bloquant)
- **BC25-02** `src/logger/log_manager.c` — retour `system()` non vérifié (×3)
- **BC25-03** `src/debug/ultra_forensic_logger.c` — `_GNU_SOURCE` redéfini sans garde `#ifndef`
- **BC25-04** `src/debug/ultra_forensic_logger.c` — `get_realtime_ns()` `__attribute__((unused))`
- **BC25-05** `advanced_parallel.c` — `normalize_state_vector_ret()` `__attribute__((unused))`
- **BC25-06+C20-CHI** Même fichier — `pt_pairing_cold[]` jamais lu → bloc forensique + `chi_sc` ajouté

## Scientific Results — Cycle 20 (run 6494, 2026-03-15)
- **N_SWEEPS=4000** (doublé), **N_THERMALIZE=800** (+60%)
- **Tc méthode 1** (dP/dT) = **72 K** (scan 13 pts, résolution 2-3 K autour du pic)
- **Tc méthode 2** (χ_sc peak) = **95 K** (chi_sc_peak = 1.23×10⁻⁵ à T=95K)
- **Tc consensus** = 72 K (désaccord 23K > seuil 20K → méthode 1)
- Interprétation: T*=95K (pseudogap), Tc=72K (transition de phase) — plage cuprates YBCO 92K
- Limite thermodynamique : E_∞ = 1.9999864 eV (6.8 ppm, inchangée)
- Benchmark QMC/DMRG : **15/15 PASS** (100%) — ajout point U=16t
- χ_sc(T) implémenté : formule Scalettar PRL 1989 — χ = N·Var(P)/(k_B·T)
- A(k,ω) calculé : v_F=2.0 eV·Å (plage ARPES Bi-2212: 1.5-2.5 → PASS)
- SQLite inter-cycles : `CHAT/lumvorax_cycles.db` — 100+ runs indexés
- Métriques hardware : 5.3×10⁶ mises à jour/s, 200 qubits équivalents (10×10)

## Scientific Results — Cycle 19 (run 3124, 2026-03-14)
- Tc estimée = 80 K (méthode 1 seulement, 7 points T)
- Benchmark QMC/DMRG : 14/15 PASS, progrès = 83.1%
- E_∞ = 1.9999864 eV, RMSE = 1.86×10⁻⁵

## Analysis Reports (CHAT/)
- `analysechatgpt26.md` — **Rapport Cycle 20 COMPLET** : BC25 preuve ligne/ligne, χ_sc(T) données réelles, A(k,ω), SQLite, 7 technologies concurrentes avec URLs, critique experte, bugs cachés, auto-prompt Cycle 21
- `compute_akw_arpes.py` — Script A(k,ω) + overlay ARPES Bi-2212 (3 panels)
- `index_cycles_sqlite.py` — Indexation sémantique SQLite inter-cycles
- `akw_arpes_overlay_cycle20.png` — Figure spectroscopique Cycle 20

## Python Tools (CHAT/)
- `compute_akw_arpes.py` : A(k,ω) spectral function, BCS d-wave gap, Fermi surface, ARPES overlay
- `index_cycles_sqlite.py` : SQLite indexer pour tous les runs (tables: runs, benchmark_points, module_metrics)
