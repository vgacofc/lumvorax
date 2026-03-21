# Dépendances potentiellement manquantes à ajouter dans `nx47-dependencies`

Basé sur `pyproject.toml` (V7 Python pur) vs contenu dataset actuel fourni.

## Déjà couvert (présents ou partiellement présents)
- imagecodecs
- numpy
- opencv-python
- pillow
- scikit-image
- scipy
- tifffile

## Manquants à prévoir dans le dataset
- aiohttp
- aristotlelib
- bitstring
- kaggle
- matplotlib
- pandas
- psutil
- pyarrow
- requests
- torch

## Dépendances transitives utiles (souvent requises par les paquets ci-dessus)
- imageio (déjà présent)
- lazy_loader (déjà présent)
- networkx (déjà présent)
- packaging (déjà présent)
- python-dateutil
- pytz / tzdata
- urllib3
- certifi
- charset-normalizer
- idna
- filelock
- typing-extensions
- sympy
- jinja2
- fsspec

> Note: les versions exactes des wheels doivent être figées lors de la génération (cp311 manylinux compatibles Kaggle).
