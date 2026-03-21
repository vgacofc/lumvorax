# NX-6 — Cahier des charges expérimental

## Objectif
Instrumenter **définitivement** les mécanismes de communication, mémoire et pré‑cognition du neurone NX, en distinguant causalité physique, corrélation statistique et artefacts numériques.

## Hypothèses à tester (non supposées vraies)
H1 — La communication NX est **régimique dissipative**, non symbolique.
H2 — La mémoire NX est une **trace statistique d’invariants**, sans stockage explicite.
H3 — Les spikes/impulsions sont **émergents**, non causaux.
H4 — La fonctionnalité maximale existe dans une **fenêtre critique d’instabilité contrôlée**.

## Axes expérimentaux
### A. Canal de communication
- Perturbations locales contrôlées (énergie, bruit, topologie)
- Mesure du délai de propagation **de régime** (pas de signal)
- Ablation des courants électriques vs invariants

### B. Mémoire & hystérésis
- Cycles charge/décharge ATP
- Tests de réversibilité partielle
- Distance statistique entre distributions post‑événement

### C. Pré‑cognition
- Détection d’anticipation de transition
- Corrélations invariants → futur régime
- Faux positifs / négatifs

### D. Robustesse & pathologies
- Fenêtre fonctionnelle
- Bascule chaos / inertie
- Dépendance aux conditions initiales

## Tests unitaires obligatoires
- TU‑COMM‑01 : suppression du canal électrique
- TU‑COMM‑02 : bruit blanc vs bruit structuré
- TU‑MEM‑01 : retour paramètres globaux identiques ≠ état identique
- TU‑PRE‑01 : prédiction aveugle de transition
- TU‑PATH‑01 : stabilité excessive = perte fonctionnelle

## Logs & forensics (obligatoires)
- Logs horodatés UTC (.log)
- États sérialisés (.json)
- Courbes hystérésis (.csv)
- Checksums SHA‑256
- Signal de fin explicite

## Critères de validation
- Effet reproductible multi‑runs
- Invariance aux seeds
- Séparation claire corrélation / causalité

## Livrables
- RAPPORT_IAMO3/NX/NX‑6_*.md
- logs_AIMO3/nx/NX‑6_*.log
- datasets bruts (.csv/.json)

## Hors‑scope
- Cognition symbolique
- Apprentissage supervisé
- Optimisation globale

