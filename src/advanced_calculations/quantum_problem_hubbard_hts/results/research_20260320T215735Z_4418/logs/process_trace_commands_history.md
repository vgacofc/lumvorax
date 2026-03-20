# Process Trace — Command-by-command

## Pré-exécution locale
1. `git fetch origin --prune`
2. `bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh`
3. `python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_authenticity_audit.py src/advanced_calculations/quantum_problem_hubbard_hts <RUN_DIR>`

## Vérifications de validation
4. `cat <RUN_DIR>/tests/integration_gate_summary.csv`
5. `cat <RUN_DIR>/tests/integration_physics_gate_summary.csv`
6. `cat <RUN_DIR>/tests/integration_code_authenticity_audit.csv`
7. `cat <RUN_DIR>/tests/integration_hardcoding_risk_register.csv`
8. `sha256sum -c <RUN_DIR>/logs/checksums.sha256`
