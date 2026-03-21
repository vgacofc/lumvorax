# Audit complet V6 → V7 (Lumvorax)

## Portée et méthode
- Scan exhaustif des fichiers via index local (`docs/audit/file_inventory.tsv`).
- Génération d'un arbre de répertoires de substitution (`tree_like_L2.txt` et `tree_like_L4.txt`) car la commande `tree` n'est pas disponible dans l'environnement.
- Analyse ciblée du notebook de base V6: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v6/nx46-vx-unified-kaggle-v6.ipynb`.
- Révision des recommandations pour contrainte **intégration 100% Python** validable dans `RAPPORT-VESUVIUS/validation_lumvorax` (sans dépendance C/C++ custom à compiler dans Kaggle).

## Chiffres globaux (A→Z, sans exclusion)
- Nombre total de fichiers: **33242**.
- Nombre de fichiers Markdown (`.md`): **1159**.
- Top extensions: `.py`=11741, `.h`=10524, `.pyi`=2058, `<no_ext>`=2003, `.md`=1159, `.txt`=756, `.tif`=753, `.so`=622, `.c`=552, `.blob`=548, `.png`=250, `.json`=222, `.mat`=110, `.csv`=106, `.cpp`=92.

## Diagnostic par sous-projet (racine)
| Sous-projet | Nb fichiers | Signatures technologiques observées (top extensions) |
|---|---:|---|
| `.venv` | 24790 | .py:10729, .h:9809, .pyi:1260, <no_ext>:791, .so:622 |
| `.pythonlibs` | 1859 | .py:804, .pyi:798, <no_ext>:118, .c:59, .txt:25 |
| `RAPPORT-VESUVIUS` | 1595 | .tif:751, .c:177, .h:149, .md:140, .py:73 |
| `.ccls-cache` | 1096 | .blob:548, .h:462, .c:37, .tcc:20, .0@bit:1 |
| `test_persistence.db` | 1000 | <no_ext>:1000 |
| `attached_assets` | 629 | .txt:422, .png:175, .lean:8, .py:6, .ipynb:5 |
| `src` | 396 | .c:178, .h:80, .o:43, .lean:30, .cpp:27 |
| `RAPPORT_IAMO3` | 324 | .md:268, .txt:42, .json:8, .sh:3, .png:2 |
| `reports` | 265 | .md:256, .json:3, .csv:3, <no_ext>:1, .dat:1 |
| `logs_AIMO3` | 163 | .csc:60, .json:42, .csv:36, .md:13, .bin:6 |
| `trou_noir_sim` | 59 | .c:20, <no_ext>:12, .md:10, .h:4, .json:4 |
| `.local` | 32 | .bin:30, <no_ext>:1, .json:1 |
| `evidence` | 27 | .txt:25, .json:2 |
| `kaggle_outputs` | 26 | .json:11, .bin:8, .parquet:7 |
| `PREUVE_IAMO` | 22 | .txt:15, .md:7 |
| `logs` | 20 | .txt:15, .json:1, .parquet:1, <no_ext>:1, .c:1 |
| `RAPPORT` | 18 | .md:18 |
| `dataset` | 16 | .py:10, .csv:3, .zip:1, .pdf:1, .proto:1 |
| `DATASET` | 16 | .py:10, .csv:3, .pdf:1, .txt:1, .proto:1 |
| `v28_forensic_logs` | 16 | .json:8, .bin:8 |
| `DÉSACTIVÉ` | 15 | .c:7, .h:7, .md:1 |
| `nx47_dependencies_v3` | 13 | .whl:12, .json:1 |
| `lum-vorax-dependencies` | 11 | .whl:10, .json:1 |
| `tests` | 11 | .py:11 |
| `temp_v1_v2` | 10 | .whl:10 |
| `results` | 9 | .json:8, .csv:1 |
| `kaggle_kernels` | 8 | .json:4, .py:4 |
| `v44v1` | 8 | .md:2, .png:2, .sha512:2, .json:1, .csv:1 |
| `forensic_analysis_nx47_v2` | 7 | .md:7 |
| `iamo3_results` | 7 | .json:3, .bin:3, .parquet:1 |
| `nfl_results` | 7 | .json:3, .bin:3, .parquet:1 |
| `modules` | 6 | .py:5, .json:1 |
| `build_kaggle` | 5 | .json:3, .md:1, .py:1 |
| `final_v4_output` | 5 | .json:2, .bin:2, .parquet:1 |
| `iamo3_results_v2` | 5 | .json:2, .bin:2, .parquet:1 |

## État de la base V6 (détection de fonctionnalités perf)
- ❌ `torch.compile`
- ❌ `autocast`
- ❌ `GradScaler`
- ❌ `channels_last`
- ❌ `pin_memory`
- ❌ `prefetch_factor`
- ❌ `persistent_workers`
- ❌ `num_workers`
- ❌ `cudnn.benchmark`
- ❌ `torch.jit`
- ❌ `onnx`
- ❌ `tensorrt`
- ❌ `xformers`
- ❌ `flash_attn`
- ❌ `numba`
- ❌ `nvidia.dali`
- ❌ `dask`
- ❌ `polars`
- ❌ `zarr`
- ❌ `memmap`
- ❌ `parquet`
- ✅ `cupy`
- ✅ `cupyx`
- ✅ `ray`
- ✅ `tta`
- ✅ `ensemble`

## Décision d'intégration V7 (contrainte: Python pur Kaggle)
### Ce qui est écarté pour V7 (trop complexe / fragile en environnement Kaggle)
1. Intégration directe des modules C/C++ custom de `src/optimization/*` (mmap C, lockfree C, simd C, lz4 C) via compilation runtime.
2. Binding custom `ctypes/cffi/pybind11` à maintenir en notebook.
3. Dépendances GPU avancées non garanties (`TensorRT`, `xformers`, `flash_attn`, `nvidia.dali`).

### Ce qui est retenu pour V7 en Python pur
1. **Optimisation PyTorch native**: `torch.compile` (fallback automatique), AMP (`torch.cuda.amp.autocast` + `GradScaler`), `channels_last`, `cudnn.benchmark`.
2. **Data pipeline Python**: tuning DataLoader (`num_workers`, `pin_memory`, `persistent_workers`, `prefetch_factor`) et batching adaptatif selon VRAM.
3. **I/O Python**: préprocessing en `numpy.memmap`, cache de features en `parquet/pyarrow` et/ou `zarr`.
4. **Parallélisme Python contrôlé**: `concurrent.futures` / multiprocessing léger pour précompute CPU, sans cluster distribué imposé.
5. **Profiling Python**: instrumentation simple (`time.perf_counter`, `torch.cuda.synchronize`, `psutil`) et tableau comparatif baseline vs optimisé.

## Plan V7 recommandé (100% Python)
### P0 (impact immédiat)
- Activer AMP + `torch.compile` + `channels_last` avec flags de fallback.
- Tuner DataLoader avec mini benchmark local (3-5 configs) et garder la meilleure.
- Ajouter cache `numpy.memmap` des patches TIF pour éviter les relectures complètes.

### P1 (impact fort, effort moyen)
- Ajouter pipeline de features intermédiaires en `parquet` (pyarrow) avec versioning simple.
- Ajouter option `zarr` pour accès chunké quand le dataset dépasse la RAM.
- Mettre en place TTA/ensemble conditionnels (activés seulement si gain net mesuré).

### P2 (stabilisation)
- Ajouter un script de benchmark reproductible (temps I/O, temps inférence, RAM/VRAM).
- Ajouter garde-fous d'environnement Kaggle (désactiver automatiquement les options indisponibles).

## Checklist d'intégration pour `validation_lumvorax`
- [ ] Aucun build C/C++ requis.
- [ ] Notebook exécutable de bout en bout avec dépendances Python existantes.
- [ ] Fallback automatique CPU/GPU.
- [ ] Temps d'exécution mesuré avant/après optimisation.
- [ ] Export final de métriques dans un artefact `.json`.


## Vérification dataset dépendances (`nx47-dependencies`)
### État actuel connu
Le contenu actuellement présent a été consigné dans `docs/audit/dependency_dataset_update/dataset_current_contents.md` pour éviter toute perte de l'existant.

### Couverture vs besoins V7 Python pur
Référence dépendances cibles: `docs/audit/dependency_dataset_update/requirements_v7_python_pure.txt`.
Écart (manquants + transitifs conseillés): `docs/audit/dependency_dataset_update/missing_from_dataset.md`.

### Liste à compléter pour V7 (priorité installation)
- `aiohttp`
- `aristotlelib`
- `bitstring`
- `kaggle`
- `matplotlib`
- `pandas`
- `psutil`
- `pyarrow`
- `requests`
- `torch`

### Traçabilité et génération Replit
Procédure prête à l'emploi: `docs/audit/dependency_dataset_update/update_commands.md`.
Cette procédure crée un dossier versionnable `build_kaggle/dependency_bundle_v7/` contenant:
- snapshot documentaire,
- wheels téléchargés,
- manifeste `MANIFEST_SHA256.json` (hash + taille),
- base prête pour push Kaggle dataset (sans supprimer l'existant).

## Livrables générés
- `docs/audit/file_inventory.tsv` (inventaire exhaustif fichier par fichier).
- `docs/audit/md_inventory.txt` (liste exhaustive `.md`).
- `docs/audit/tree_like_L2.txt` et `docs/audit/tree_like_L4.txt` (arborescences).
- `docs/v7_audit_inventory.json` (statistiques structurées).
- `docs/audit/dependency_dataset_update/` (kit de traçabilité + checklist + commandes de mise à jour dataset).

## Diagnostic complémentaire demandé — résultats V6 (`result-nx46-vx-v6`)

### 1) Vérification de la source analysée
- Source de référence confirmée: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v6/nx46-vx-unified-kaggle-v6.ipynb`.
- La version V5 demandée est également présente pour comparaison: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v5/nx46-vx-unified-kaggle-v5.ipynb`.

### 2) Conformité globale observée (par rapport aux analyses précédentes)
- Le run V6 contient bien:
  - pipeline de progression détaillé (`PROGRESS_UPDATE`),
  - offline dependency manifest,
  - génération `submission.zip`,
  - chaîne forensic/Merkle.
- Les marqueurs meta-neuronaux clés sont présents: `meta_neurons=3`, `ratio_candidates`, `threshold_scan`, `use_unet_25d`, et logs de calibration/entraînement.

### 3) Pourquoi "ça redémarre à 1" vers `28880.0s` ?
Interprétation technique la plus probable (et cohérente avec le code/logs V6):
1. Le champ affiché "1" dans la vue console est un **compteur de ligne/stream d'affichage**, pas le `index` interne de progression métier.
2. Dans les événements `PROGRESS_UPDATE`, les compteurs métier restent cohérents par sous-étape (`stage/substage`), et les reset sont normaux lorsqu'on change de boucle locale (ex: nouveau neurone, nouvelle sous-phase, nouveau mini-cycle de fit).
3. Le `global_progress_percent` reste stable pendant des micro-boucles internes (ex: `train_supervised -> fit_kpi`), ce qui donne visuellement l'impression d'un "redémarrage" alors qu'il s'agit d'une sous-boucle.

### 4) Ce qu'il faut ajouter dans l'audit V7 pour éviter cette confusion
Ajouter explicitement dans V7:
- un identifiant hiérarchique de progression: `global_step_id`, `stage_step_id`, `subloop_step_id`.
- un compteur monotone unique pour l'UI: `ui_seq_monotonic`.
- une section de logs "progress semantics" expliquant quels champs peuvent se réinitialiser (index local) et lesquels sont monotones (ts_ns, ui_seq_monotonic).
- export `progress_timeline.parquet/jsonl` pour post-analyse (group by stage/substage/neuron/epoch).

### 5) Delta V5→V6 à conserver pour V7 (constaté)
- V6 ajoute un patch runtime GPU policy (sélection H100/P100 + variables d'environnement dédiées) tout en conservant le noyau meta-neuronal V5.
- Conclusion: pour V7, conserver le noyau décisionnel V5/V6, et améliorer la lisibilité de télémétrie (pas seulement la performance brute).

### 6) Limite de synchronisation dépôt distant
- Tentative de mise à jour depuis `https://github.com/lumc01/Lumvorax.git` effectuée, mais impossible dans cet environnement (`CONNECT tunnel failed, response 403`).
- Analyse réalisée sur l'état local présent dans la workspace.
