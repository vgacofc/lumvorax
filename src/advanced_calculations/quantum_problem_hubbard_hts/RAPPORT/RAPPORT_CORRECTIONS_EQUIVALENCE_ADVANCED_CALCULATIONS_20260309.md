# Rapport d'exécution immédiate — corrections demandées vs code réel du dépôt

## Constat de localisation
Les chemins cités dans la demande ne sont pas présents tels quels dans ce dépôt :
- `hubbard_hts_core/evolution.c`
- `qcd_lattice_proxy/montecarlo.cpp`
- `dense_nuclear_proxy/dynamics.f90`
- `quantum_field_noneq/solver.py`
- `quantum_chemistry_proxy/hamiltonian.cpp`

Le moteur réel actif est regroupé principalement dans :
- `src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_module.c`
- `src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c`
- `src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c`

## Corrections appliquées immédiatement dans les équivalents réels

### 1) Projection / normalisation d'état ajoutée
- Ajout de `normalize_state_vector(double* d, int n)` dans les 3 moteurs C.
- Appel dans les boucles d'évolution à chaque step.

### 2) Instrumentation HFBL native en runtime
- Journalisation événementielle native C (`simulation_start`, `simulation_step`, `simulation_end`) avec :
  - `simulation_step`
  - `energy_update`
  - `observable_update`
  - `sign_ratio`
  - `state_hash`
- Capture active dès le début de campagne + checkpoint de fin.

### 3) Correctif de persistance forensic
- Dans le logger Python HFBL, le test de persistance passe en mode append (`"a"`) pour ne plus écraser les traces runtime natives.

## Résultat
Les corrections critiques demandées (activation forensic début de flux, projection/normalisation d'état, suppression de la logique purement cumulative déjà traitée précédemment) sont maintenant appliquées sur le code réellement exécuté du dépôt.
