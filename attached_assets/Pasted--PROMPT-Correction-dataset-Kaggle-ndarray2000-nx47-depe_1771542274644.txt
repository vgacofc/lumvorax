# PROMPT — Correction dataset Kaggle `ndarray2000/nx47-dependencies` (Version 3 complète)

Tu dois corriger le dataset Kaggle **sans perdre les anciennes dépendances**.
Objectif: publier une **Version 3** qui contient l'union complète de V1 + V2.

## 1) Source de vérité (fichiers exacts à inclure)

### V1 (à réintégrer explicitement si absents)
- `imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `tifffile-2026.1.28-py3-none-any.whl`

### V2 (déjà publiés)
- `imageio-2.37.2-py3-none-any.whl`
- `lazy_loader-0.4-py3-none-any.whl`
- `liblumvorax_replit.so`
- `networkx-3.6.1-py3-none-any.whl`
- `numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `opencv_python-4.13.0.92-cp37-abi3-manylinux_2_28_x86_64.whl`
- `packaging-26.0-py3-none-any.whl`
- `pillow-12.1.1-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `scikit_image-0.26.0-cp311-cp311-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl`
- `scipy-1.17.0-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `tifffile-2026.2.16-py3-none-any.whl`

## 2) Résultat attendu en V3
Le dataset `ndarray2000/nx47-dependencies` doit contenir **au minimum** les 12 artefacts suivants (union V1+V2, en gardant les deux versions de tifffile):

1. `imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
2. `imageio-2.37.2-py3-none-any.whl`
3. `lazy_loader-0.4-py3-none-any.whl`
4. `liblumvorax_replit.so`
5. `networkx-3.6.1-py3-none-any.whl`
6. `numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
7. `opencv_python-4.13.0.92-cp37-abi3-manylinux_2_28_x86_64.whl`
8. `packaging-26.0-py3-none-any.whl`
9. `pillow-12.1.1-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
10. `scikit_image-0.26.0-cp311-cp311-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl`
11. `scipy-1.17.0-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
12. `tifffile-2026.1.28-py3-none-any.whl`
13. `tifffile-2026.2.16-py3-none-any.whl`

> Important: ne supprime rien tant que la compatibilité n'est pas validée. Ici on veut corriger la régression de dépendances manquantes, pas minimiser le dataset.

## 3) Emplacements exacts
- Dataset Kaggle cible: `ndarray2000/nx47-dependencies`
- Emplacement runtime attendu par le kernel: `/kaggle/input/datasets/ndarray2000/nx47-dependencies`
- Fallback runtime: `/kaggle/input/nx47-dependencies`

## 4) Procédure opératoire (commandes)

```bash
# Préparer un dossier local avec TOUS les artefacts V3
mkdir -p ./nx47_dependencies_v3
# (copier tous les fichiers listés ci-dessus dans ce dossier)

# Vérifier localement la présence des fichiers attendus
python3 RAPPORT-VESUVIUS/validation_lumvorax/verify_dependency_bundle_v3.py --dir ./nx47_dependencies_v3

# Publier la V3 sur Kaggle (metadata doit pointer vers ndarray2000/nx47-dependencies)
kaggle datasets version -p ./nx47_dependencies_v3 -m "V3 restore missing V1 deps (imagecodecs + tifffile-2026.1.28) and keep V2 artifacts"

# Contrôle post-publication
kaggle datasets files ndarray2000/nx47-dependencies --page-size 200
```

## 5) Critères GO/NO-GO
- GO uniquement si:
  - `imagecodecs-2026.1.14-...whl` est présent,
  - `tifffile-2026.1.28-py3-none-any.whl` est présent,
  - les artefacts V2 restent présents,
  - `verify_dependency_bundle_v3.py` retourne OK.
- NO-GO si un fichier de la liste ci-dessus manque.
