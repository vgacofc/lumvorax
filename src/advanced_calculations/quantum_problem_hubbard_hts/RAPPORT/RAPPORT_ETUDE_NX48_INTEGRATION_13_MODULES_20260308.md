# Étude séparée — Faisabilité intégration NX-48 sur les 13 simulations Hubbard HTS

## 1) Portée et contrainte
- Étude **sans intégration logicielle** dans le moteur actuel (conforme à la demande).
- Objectif: vérifier si NX-48 peut piloter dynamiquement **tous les paramètres existants** des 13 modules.

## 2) Inventaire exhaustif des paramètres actuels (code source)
- Paramètres structure `problem_t`: `name`, `lx`, `ly`, `t`, `u`, `mu`, `temp`, `dt`, `steps`.
- Paramètres contrôle dynamique `control_flags_t`: `phase_control`, `resonance_pump`, `magnetic_quench`, `phase_step`, `phase_field`, `pump_gain`, `quench_strength`.
- Observables de sortie `sim_result_t`: `energy`, `pairing`, `sign_ratio`, `cpu_peak`, `mem_peak`, `elapsed_ns`.

### 2.1 Paramètres par module (13/13)
| Module | lx | ly | t | U | mu | temp | dt | steps | U/t | volume |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| hubbard_hts_core | 10 | 10 | 1.0 | 8.0 | 0.2 | 95.0 | 0.01 | 2800 | 8.000000 | 100 |
| qcd_lattice_proxy | 9 | 9 | 0.7 | 9.0 | 0.1 | 140.0 | 0.01 | 2200 | 12.857143 | 81 |
| quantum_field_noneq | 8 | 8 | 1.3 | 7.0 | 0.05 | 180.0 | 0.01 | 2100 | 5.384615 | 64 |
| dense_nuclear_proxy | 9 | 8 | 0.8 | 11.0 | 0.3 | 80.0 | 0.01 | 2100 | 13.750000 | 72 |
| quantum_chemistry_proxy | 8 | 7 | 1.6 | 6.5 | 0.4 | 60.0 | 0.01 | 2200 | 4.062500 | 56 |
| spin_liquid_exotic | 12 | 10 | 0.9 | 10.5 | 0.12 | 55.0 | 0.01 | 2600 | 11.666667 | 120 |
| topological_correlated_materials | 11 | 11 | 1.1 | 7.8 | 0.15 | 70.0 | 0.01 | 2500 | 7.090909 | 121 |
| correlated_fermions_non_hubbard | 10 | 9 | 1.2 | 8.6 | 0.18 | 85.0 | 0.01 | 2400 | 7.166667 | 90 |
| multi_state_excited_chemistry | 9 | 9 | 1.5 | 6.8 | 0.22 | 48.0 | 0.01 | 2300 | 4.533333 | 81 |
| bosonic_multimode_systems | 10 | 8 | 0.6 | 5.2 | 0.06 | 110.0 | 0.01 | 2200 | 8.666667 | 80 |
| multiscale_nonlinear_field_models | 12 | 8 | 1.4 | 9.2 | 0.10 | 125.0 | 0.01 | 2300 | 6.571429 | 96 |
| far_from_equilibrium_kinetic_lattices | 11 | 9 | 1.0 | 8.0 | 0.09 | 150.0 | 0.01 | 2400 | 8.000000 | 99 |
| multi_correlated_fermion_boson_networks | 10 | 10 | 1.05 | 7.4 | 0.14 | 100.0 | 0.01 | 2350 | 7.047619 | 100 |

## 3) Faisabilité NX-48 (expert)
**Conclusion courte:** faisable techniquement, sous réserve d’une couche adaptateur stricte entre NX-48 et le moteur proxy déterministe actuel.

### 3.1 Ce qui est déjà favorable
- Espace des paramètres est **structuré et bornable** (géométrie + couplages + contrôle dynamique).
- Le moteur expose déjà des métriques de feedback exploitables pour apprentissage: énergie, pairing, sign ratio, stabilité, temps, mémoire.
- Les 13 modules partagent une signature paramétrique homogène, facilitant un policy model unique multi-tâches.

### 3.2 Risques techniques à traiter avant connexion
- Non-stabilité numérique observée (drift énergie / Von Neumann) peut contaminer la récompense NX-48.
- Références benchmark externes encore en échec: risque d’optimiser un objectif local non physique.
- Simulateur actuel principalement déterministe par seed; NX-48 doit intégrer exploration contrôlée pour éviter sur-apprentissage.

## 4) Architecture proposée (sans implémentation ici)
1. **Adapter Layer**: mappe paramètres NX-48 -> `problem_t` + `control_flags_t` avec bornes strictes.
2. **Safety Layer**: garde-fous hard (bornes dt/U/t/temp, mémoire max, timeout, stabilité).
3. **Evaluator Layer**: calcule score composite (convergence + stabilité + benchmark + coût).
4. **Replay Store**: historise `(paramètres, observables, score)` pour apprentissage hors-ligne et audit.
5. **Policy Update Loop**: ajuste NX-48 en batch, jamais en écriture directe sur le moteur sans validation.

## 5) Mapping paramètre -> entrée/sortie NX-48
- Entrées minimales: `[lx, ly, t, U, mu, temp, dt, steps, phase_control, resonance_pump, magnetic_quench, phase_step, phase_field, pump_gain, quench_strength]`.
- Signaux d’état: `[energy_norm, pairing_norm, sign_ratio, spectral_radius, energy_drift_max, cpu_peak, mem_peak]`.
- Actions NX-48: ajustements continus bornés de `dt`, `phase_field`, `pump_gain`, `quench_strength`, et éventuellement `temp`, `U`, `mu` selon protocole expérimental.
- Récompense proposée: maximiser `pairing` sous contraintes `spectral_radius<=1`, `energy_drift<seuil`, `benchmark_error` minimal, coût CPU/RAM borné.

## 6) Plan de déploiement sans casse
- Phase A: Shadow mode (NX-48 propose, moteur n’applique pas).
- Phase B: Assisted mode (application sur sous-ensemble modules stables).
- Phase C: Full adaptive mode (13 modules), seulement après PASS stabilité + benchmark.
- Rollback obligatoire: retour instantané aux paramètres baseline en cas de fail stabilité.

## 7) Traduction pédagogique
- NX-48 peut être vu comme un “pilote automatique” qui teste des réglages puis garde ceux qui améliorent les résultats **sans casser la stabilité**.
- Aujourd’hui, ce pilote est possible, mais il faut d’abord fiabiliser les capteurs (tests de stabilité/benchmark), sinon il apprendrait sur des signaux trompeurs.

## 8) Verdict final
- **Oui**, connexion NX-48 à tous les paramètres des 13 modules est faisable.
- **Condition bloquante**: mettre d’abord les métriques de stabilité/benchmark sous contrôle pour éviter l’optimisation d’artefacts.
- Cette étude reste volontairement séparée des implémentations actuelles, comme demandé.
