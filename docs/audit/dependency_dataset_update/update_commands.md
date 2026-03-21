# Commandes Replit pour générer le dossier de push dataset (traçabilité)

## 1) Préparer le dossier de sortie
```bash
mkdir -p build_kaggle/dependency_bundle_v7
cp docs/audit/dependency_dataset_update/* build_kaggle/dependency_bundle_v7/
```

## 2) Télécharger les wheels Python purs
```bash
python -m pip download \
  -r docs/audit/dependency_dataset_update/requirements_v7_python_pure.txt \
  --dest build_kaggle/dependency_bundle_v7/wheels \
  --only-binary=:all:
```

## 3) Générer un manifeste de traçabilité
```bash
python - <<'PY'
import hashlib, json, pathlib
root = pathlib.Path('build_kaggle/dependency_bundle_v7')
rows = []
for p in sorted(root.rglob('*')):
    if p.is_file():
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        rows.append({'path': str(p.relative_to(root)), 'size': p.stat().st_size, 'sha256': h})
out = root / 'MANIFEST_SHA256.json'
out.write_text(json.dumps(rows, indent=2), encoding='utf-8')
print('manifest:', out)
PY
```

## 4) Fusion avec l'existant dataset avant push
- Conserver tous les artefacts déjà présents.
- Ajouter uniquement les wheels/manifestes manquants.
- Mettre à jour la description dataset avec la liste des nouveautés.

## 5) Push Kaggle dataset (exemple)
```bash
kaggle datasets version -p build_kaggle/dependency_bundle_v7 \
  -m "V7 deps update: add missing python wheels + manifest"
```
