# PROMPT AGENT REPLIT — SYNCHRONISATION DÉPÔT DISTANT + FICHIERS EXACTS (LUMVORAX V13)

Tu dois d’abord synchroniser et inspecter le dépôt distant puis corriger localement avec des chemins **exacts**.

## 0) SOURCE DE VÉRITÉ DISTANTE
Dépôt: `https://github.com/lumc01/Lumvorax.git`
Branche à inspecter en priorité: `origin/replit-agent`

Commandes obligatoires:
```bash
git remote get-url origin
git fetch origin --prune
git checkout work
git log --oneline -n 20 origin/replit-agent
git ls-tree -r --name-only origin/replit-agent
```

## 1) FICHIERS EXACTS À INSPECTER CÔTÉ DISTANT (REPLIT-AGENT)
Inspecter **exactement** ces fichiers du distant:

### 1.1 Noyau C Lumvorax (remote)
- `src/lum/lum_core.c`
- `src/lum/lum_core.h`
- `src/vorax/vorax_operations.c`
- `src/vorax/vorax_operations.h`
- `src/parser/vorax_parser.c`
- `src/parser/vorax_parser.h`
- `src/binary/binary_lum_converter.c`
- `src/binary/binary_lum_converter.h`
- `src/crypto/crypto_validator.c`
- `src/crypto/crypto_validator.h`
- `src/logger/lum_logger.c`
- `src/logger/lum_logger.h`
- `src/metrics/performance_metrics.c`
- `src/metrics/performance_metrics.h`
- `src/optimization/memory_optimizer.c`
- `src/optimization/memory_optimizer.h`
- `src/parallel/parallel_processor.c`
- `src/parallel/parallel_processor.h`
- `src/persistence/data_persistence.c`
- `src/persistence/data_persistence.h`
- `src/main.c`

### 1.1.b INVENTAIRE TOTAL OBLIGATOIRE (RACINE + DISTANT)
Tu dois scanner **toi-même** la racine et produire l’inventaire complet des modules/sous-modules avant correction.

Commandes obligatoires (sans suppression):
```bash
pwd
rg --files src
rg --files | rg '^RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle/src/.*\.(c|h)$'
```

Livrable obligatoire (append-only):
- `RAPPORT-VESUVIUS/validation_lumvorax/MODULES_LUMVORAX_INVENTAIRE_TOTAL_V13.md`
- Inclure:
  - tous les chemins `src/**/*.c` et `src/**/*.h` trouvés,
  - regroupement par module,
  - fonctions exportées attendues vs trouvées,
  - manquants bloquants (ex: `neural_config_create_default`).

### 1.2 Tests distants existants
- `src/tests/test_lum_core.c`
- `src/tests/test_advanced_modules.c`

### 1.3 Build distant
- `Makefile`
- `metadata.json`
- `README.md`
- `replit.md`

## 2) FICHIERS EXACTS À METTRE À JOUR CÔTÉ LOCAL (KAGGLE/DATASET)
Tu modifies **uniquement** ces fichiers cibles (sauf nécessité prouvée):

### 2.1 Validateur Kaggle
- `build_kaggle/kernel/main.py`
- `build_kaggle/kernel/kernel-metadata.json`
- `build_kaggle/bundle/dataset-metadata.json`

### 2.2 Notebook/Script de validation Lumvorax dépendances
- `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V6_BINARY_20260221.py`

### 2.3 Bundle dataset NX47 dépendances
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle/`
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle/dataset-metadata.json`
- Wheels dans `.../bundle/*.whl`
- Binaire natif attendu: `.../bundle/liblumvorax.so`

## 3) CHECKLIST TECHNIQUE OBLIGATOIRE (ZÉRO WARNING)

### 3.1 ABI / Wheels
- Python runtime Kaggle exact.
- Vérifier tags `sys_tags()`.
- Vérifier chaque wheel du dataset:
  - package présent,
  - compatibilité tag,
  - pas de paquet requis manquant/incompatible.

### 3.2 SO natif
- Scanner `liblumvorax.so`:
  - `nm -D` (exports + undefined),
  - `ldd` (liaisons dynamiques),
  - check symboles obligatoires module par module.
- Bloquant si symbole requis absent (ex: `neural_config_create_default`).
- Vérifier explicitement les 42 modules du contrat `MODULE_SYMBOL_REQUIREMENTS` dans `build_kaggle/kernel/main.py`.
- **En plus**, vérifier l’inventaire complet réel des modules/sous-modules trouvés dans les sources (objectif: >75 modules).
- Exporter deux tables finales:
  - `module_42 | symbole_requis | présent/absent`
  - `module_source | fichier(s) | présent/absent_dans_so | remarque`
- Bloquant si l’inventaire source total est `< 75 modules` (tant que les sources sont accessibles).

### 3.3 TIFF LZW
- Vérifier backend réel `imagecodecs` (`lzw_encode`, `lzw_decode`).
- Vérifier backend alternatif réel `Pillow+libtiff` si disponible.
- Exécuter roundtrip TIFF LZW réel (write/read/compare).
- Aucune validation partielle acceptée.

### 3.4 Notebook certification
Valider `LUMVORAX V7 Certification Test` (migré V13 si requis) avec artefacts Kaggle réels:
- statut final OK,
- zéro warning,
- zéro erreur.

## 4) TRAÇABILITÉ APPEND-ONLY (OBLIGATOIRE)
Ne rien supprimer dans les rapports existants.
Ajouter un addendum horodaté avec:
- commandes exactes,
- sortie utile,
- décision,
- fichiers modifiés,
- résultat après correction.

## 5) COMMANDES MINIMALES À EXÉCUTER
```bash
python -m py_compile build_kaggle/kernel/main.py
python build_kaggle/kernel/main.py
```
Puis exécution Kaggle réelle (dataset monté) jusqu’au résultat:
- `status == "ok"`
- `warnings_count == 0`
- `so_load_status == "ok"`
- `roundtrip_status == "ok"`

## 6) CONDITION DE FIN
Tu ne t’arrêtes que lorsque notebook + dataset nx47 sont validés sans erreur ni warning, avec preuves Kaggle, journal append-only complet, et inventaire source complet (>75 modules) vérifié.
