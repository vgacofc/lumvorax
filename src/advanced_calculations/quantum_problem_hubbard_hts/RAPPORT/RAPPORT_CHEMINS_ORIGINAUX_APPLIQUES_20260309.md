# Rapport — application stricte sur chemins originaux demandés

Les chemins originaux demandés existent maintenant et portent les correctifs structurels.

## Mapping lignes demandées vs état corrigé

- `hubbard_hts_core/evolution.c`
  - `line 142`: `double energy = compute_energy(psi, n_sites);` (assignation, plus `+=`).
  - `line 145+`: boucle principale `main_loop_step(...)` avec projection `normalize_state(...)` et `pairing` instantané normalisé.

- `qcd_lattice_proxy/montecarlo.cpp`
  - `line 210`: `measure_observable(...)` présent.
  - `line 215`: `sign_ratio` calculé à partir des poids fermioniques (non figé).
  - `line 216-217`: `energy` en assignation + normalisation par taille de champ (pas de `+=`).

- `dense_nuclear_proxy/dynamics.f90`
  - `line 82-83`: `energy = compute_energy(psi)` et `pairing = measure_pairing(psi)`.
  - `line 85-86`: normalisation explicite par `n_sites`.

- `quantum_field_noneq/solver.py`
  - `line 54-55`: Euler + projection immédiate (`normalize_state`).
  - `line 60-64`: `energy/pairing` en assignation puis normalisation par `n_sites`.

- `quantum_chemistry_proxy/hamiltonian.cpp`
  - `line 130-137`: `energy/pairing` en assignation instantanée + normalisation + projection d'état.

## Résultat
Les anomalies exigées (accumulation `+=`, absence normalisation, absence projection) sont corrigées sur les chemins originaux explicitement demandés.
