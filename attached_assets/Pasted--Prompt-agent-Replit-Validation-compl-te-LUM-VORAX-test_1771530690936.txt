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
