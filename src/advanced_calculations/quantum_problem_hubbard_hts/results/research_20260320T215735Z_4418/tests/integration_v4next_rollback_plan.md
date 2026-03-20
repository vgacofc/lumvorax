# V4 NEXT Rollout/Rollback Plan

- Mode demandé: `full`
- Statut: `ROLLBACK_TRIGGERED`
- Activated: `False`
- Rollback automatique: `ENABLED`

## Procédure automatique
1. Si seuils non respectés, revenir à `shadow` immédiatement.
2. Désactiver activation globale (`full`).
3. Conserver logs/checksums et artefacts de drift pour diagnostic.
4. Réexécuter après correction.
