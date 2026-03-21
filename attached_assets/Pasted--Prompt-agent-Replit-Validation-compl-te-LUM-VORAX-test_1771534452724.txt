# Prompt agent Replit — Validation complète LUM/VORAX (tests unitaires + intégration)

Tu dois exécuter **exactement** les tests préparés dans ce dossier, puis produire un rapport final lisible pour validation.

## Contexte
Le système LUM/VORAX doit être validé avec preuves machine (JSON + logs) et résumé humain.

## Étapes obligatoires
1. Ouvre et lis le script:
   - `RAPPORT-VESUVIUS/validation_lumvorax/run_lumvorax_validation.py`
2. Exécute:
   - `python3 RAPPORT-VESUVIUS/validation_lumvorax/run_lumvorax_validation.py`
3. Vérifie la présence des artefacts:
   - `RAPPORT-VESUVIUS/validation_lumvorax/validation_results.json`
   - `RAPPORT-VESUVIUS/validation_lumvorax/VALIDATION_LUMVORAX_SYSTEME_COMPLET_20260219.md`
4. Relis ces deux fichiers et produis un **rapport d'analyse** dans:
   - `RAPPORT-VESUVIUS/validation_lumvorax/RAPPORT_AGENT_REPLIT_POST_TESTS.md`

## Exigences du rapport post-tests
- Résumer test par test (source indentation, roundtrip `.lum`, intégration 3D Python, tentative compile native C).
- Indiquer clairement **confirmé** vs **en attente**.
- Lister les blocages et les actions suivantes prioritaires.
- Conserver un style pédagogique (termes techniques expliqués brièvement).

## Interdictions
- Ne pas supprimer/modifier les anciens rapports.
- Ne pas utiliser placeholders/stubs dans le rapport final.
- Ne pas masquer les erreurs: si un test échoue, explique la cause réelle.

## Vérification additionnelle obligatoire (Kaggle V2)
Après les étapes ci-dessus, vérifie explicitement la correction notebook-safe:
1. Contrôle que `validate_source_indentation()` ne dépend plus strictement de `__file__`.
2. Exécute un test simulant un contexte notebook (appel sans `source_path`) et confirme absence de `NameError`.
3. Documente dans le rapport final si le garde est:
   - actif quand un fichier est trouvable,
   - non bloquant quand aucun fichier n'est trouvable.

## Procédure anti-`IndentationError` Kaggle (obligatoire)
1. Exécuter d'abord:
   - `python3 RAPPORT-VESUVIUS/validation_lumvorax/verify_nx47_source_integrity.py`
2. Si `tabs>0` ou `ast_ok=False`, **stop** et corriger le fichier avant toute exécution.
3. Sur Kaggle, privilégier l'exécution du fichier source (pas de collage de tout le code en cellule):
   - `!python /kaggle/working/nx47_vesu_kernel_v2.py`
4. Dans le rapport final, joindre la sortie intégrale de l'étape intégrité (`sha256`, `tabs`, `ast_ok`).

## Gate obligatoire avant push dataset dépendances LUMVORAX
Tu dois valider **depuis la racine Replit** l'exécution du fichier source (pas notebook/copie cellule):
1. `python3 RAPPORT-VESUVIUS/validation_lumvorax/verify_nx47_source_integrity.py`
2. `python3 RAPPORT-VESUVIUS/validation_lumvorax/run_lumvorax_validation.py`
3. Vérifier dans `validation_results.json` que `replit_root_file_execution.ok == true`.
4. Si ce gate est faux, il est **interdit** de pousser les dépendances LUMVORAX sur dataset.

## Vérification structure code C (obligatoire)
- Confirmer explicitement dans le rapport final:
  1. si le module 3D C existe (`OUI/NON`),
  2. les chemins exacts (attendu: `src/vorax/vorax_3d_volume.c` + `.h`),
  3. le résultat de compilation locale,
  4. la conclusion GO/NO-GO pour push dataset dépendances.

## Validation complète "aucun module manquant" (obligatoire)
Exécute aussi:
1. `python3 RAPPORT-VESUVIUS/validation_lumvorax/test_vorax_3d_volume_ctypes.py`
2. `python3 RAPPORT-VESUVIUS/validation_lumvorax/run_lumvorax_validation.py`
3. Vérifie dans `validation_results.json`:
   - `native_3d_c_sources_present == true`
   - `native_compile_attempt.ok == true`
   - `replit_root_file_execution.ok == true`

Puis ajoute dans le rapport final un tableau **GO/NO-GO**:
- Module 3D C présent au bon emplacement (`src/vorax/vorax_3d_volume.c/.h`)
- Compilation native `.so`
- Exécution racine Replit
- Roundtrip `.lum`
- Détection modules manquants (si un module requis manque => NO-GO)

## Prompt final — Push dataset dépendances + activation versions (obligatoire)
Après validation GO, exécute cette séquence **sans omission**:

1. **Build dépendances LUMVORAX**
   - compiler la lib native (`liblumvorax_replit.so`) avec tous modules requis:
     - `src/vorax/vorax_operations.c`
     - `src/vorax/vorax_3d_volume.c`
     - `src/lum/lum_core.c`
     - `src/logger/lum_logger.c`
     - `src/debug/forensic_logger.c`
   - préparer les wheels Python/offline dans un dossier `lum-vorax-dependencies/`.

2. **Validation pré-push dataset (GO/NO-GO)**
   - relancer:
     - `python3 RAPPORT-VESUVIUS/validation_lumvorax/verify_nx47_source_integrity.py`
     - `python3 RAPPORT-VESUVIUS/validation_lumvorax/test_vorax_3d_volume_ctypes.py`
     - `python3 RAPPORT-VESUVIUS/validation_lumvorax/run_lumvorax_validation.py`
   - vérifier dans `validation_results.json`:
     - `native_3d_c_sources_present == true`
     - `native_compile_attempt.ok == true`
     - `replit_root_file_execution.ok == true`
   - si un critère échoue: **NO-GO dataset push**.

3. **Push dataset Kaggle dépendances**
   - publier `lum-vorax-dependencies` (wheels + `.so` + manifeste versions/checksum SHA256).
   - documenter l’ID/version dataset publié dans le rapport final.

4. **Activation sur les versions en cours**
   - vérifier/activer chargement dépendances LUMVORAX pour:
     - `nx47_vesu_kernel_v2.py`
     - `RAPPORT-VESUVIUS/notebook-version-NX47-V61.2/nx47-vesuvius-challenge-surface-detection-v61.2.py`
     - `RAPPORT-VESUVIUS/notebook-version-NX47-V61.3/nx47-vesu-kernel-new-v61-3.ipynb`
     - `RAPPORT-VESUVIUS/notebook-version-NX47-V144.1/nx47-vesu-kernel-new-v144-1.ipynb`
     - `RAPPORT-VESUVIUS/src_vesuvius/v7.5/nx46-vesuvius-core-kaggle-ready-v7.5.py`
   - confirmer `NX*_BINARY_MODE` et chemins dataset dépendances.

5. **Rapport final obligatoire**
   - générer `RAPPORT-VESUVIUS/validation_lumvorax/RAPPORT_AGENT_REPLIT_PUSH_DATASET.md`
   - inclure:
     - tableau GO/NO-GO,
     - version dataset poussée,
     - checksums,
     - preuves d’activation sur chaque version,
     - problèmes rencontrés + contournements.

## Vérification Kaggle API + dataset centralisé (nouvelle étape bloquante)
Avant push des dépendances, exécuter:
1. `kaggle --version`
2. `test -f ~/.kaggle/kaggle.json`
3. `kaggle datasets files ndarray2000/nx47-dependencies`

Critères:
- si la clé API Kaggle est absente/invalidée => **NO-GO** et demander nouvelle clé API.
- si le dataset n'est pas accessible => **NO-GO** (ne pas pousser en aveugle).
- si accès OK, vérifier que toutes dépendances requises sont listées avant upload des nouvelles versions.
