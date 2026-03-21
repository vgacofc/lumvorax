# Prompt opérationnel — Exécution immédiate Kaggle (Lumvorax V13) avec traçabilité complète

Tu es un agent d’exécution **autonome** chargé de corriger et valider intégralement l’écosystème Lumvorax sur Kaggle.

## Mission impérative
Obtenir un résultat **strictement sans erreur et sans warning** sur le notebook de certification **`LUMVORAX V7 Certification Test`** (à migrer en V13 si nécessaire), avec preuve d’exécution réelle Kaggle, et conserver une traçabilité historique exhaustive.

## Interdictions absolues
1. **Aucun placeholder**, aucun mock, aucun stub, aucune simulation.
2. **Aucun smoke test** présenté comme validation finale.
3. Ne jamais déclarer “ok” si un warning subsiste.
4. Ne jamais supprimer l’historique existant.

## Contraintes de traçabilité (append-only)
Tu dois écrire toutes tes actions dans l’historique existant **sans rien supprimer** :
- ajouter des sections datées (append-only) dans les rapports existants pertinents,
- créer un journal d’exécution détaillé horodaté si nécessaire,
- inclure chaque commande lancée, son retour, les erreurs, le correctif appliqué, puis le résultat après correctif.

Format minimal de traçabilité pour chaque étape :
- Timestamp UTC
- Objectif de l’étape
- Commande exacte exécutée
- Sortie significative (stdout/stderr)
- Décision prise
- Fichier(s) modifié(s)
- Vérification post-fix

## Plan d’exécution obligatoire (ne pas s’arrêter avant la fin)

### Phase 1 — Audit complet racine + Kaggle
1. Scanner complet du projet depuis la racine pour identifier :
   - scripts de build/publish dataset Kaggle,
   - notebook/script de certification,
   - validateurs de dépendances (`imagecodecs`, `tifffile`, `numpy`, `pillow`, etc.),
   - métadonnées Kaggle kernel/dataset.
2. Dresser la matrice de compatibilité runtime Kaggle :
   - Python exact,
   - tags ABI (`packaging.tags.sys_tags()`),
   - glibc/platform.
3. Cartographier les wheels présentes dans le dataset NX47 (toutes versions), et détecter incompatibilités ABI/tag.

### Phase 2 — Correction dépendances LZW et natif `.so`
1. Résoudre **réellement** LZW :
   - `imagecodecs` doit être installable et fonctionnel au runtime Kaggle,
   - valider `lzw_encode` et `lzw_decode`,
   - valider écriture/lecture TIFF LZW avec `tifffile`.
2. Résoudre chargement natif :
   - `liblumvorax.so` doit charger via `ctypes.CDLL` sans symbole non résolu,
   - diagnostiquer symboles undefined (`nm -D`, `ldd`, etc.),
   - rebuild/repackage si nécessaire jusqu’à chargement propre.
3. Garantir cohérence de versions de A à Z :
   - dataset Kaggle,
   - kernel metadata,
   - scripts/rapports internes.

### Phase 3 — Mise à jour certification (V7 -> V13 contrôlée)
1. Mettre à jour le test `LUMVORAX V7 Certification Test` vers état opérationnel final (nom/version/métadonnées si requis) **sans casser la compatibilité historique**.
2. Maintenir une continuité documentaire explicite : “V7 historique -> V13 strict opérationnel”.
3. S’assurer que toute référence documentaire/technique contradictoire est harmonisée.

### Phase 4 — Exécution Kaggle réelle (pas locale uniquement)
1. Publier/mettre à jour dataset dépendances sur Kaggle.
2. Pousser le kernel/notebook corrigé sur Kaggle.
3. Lancer l’exécution Kaggle complète.
4. Récupérer artefacts de sortie : logs, JSON report, statuts.

### Phase 5 — Boucle de correction jusqu’à zéro warning
Répéter tant que non conforme :
- détecter erreur/warning,
- corriger,
- republier,
- relancer Kaggle,
- revalider.

**Condition d’arrêt unique** :
- `status == "ok"`
- `warnings_count == 0`
- `error == null` (ou absent)
- roundtrip LZW = OK
- so_load = OK
- aucun package requis manquant/incompatible

## Livrables obligatoires
1. **Prompt d’exécution appliqué** (ce document) + preuve d’utilisation.
2. **Journal historique append-only** détaillé des actions (sans suppression de contenu existant).
3. **Rapport final de validation Kaggle** avec :
   - versions finales retenues,
   - raisons de compatibilité,
   - preuves d’exécution Kaggle,
   - verdict final sans warning.
4. **Table de traçabilité des modifications** : fichier -> raison -> impact -> preuve.

## Exigences qualité finales
- Aucune ambiguïté “probable”, “peut-être”, “à vérifier plus tard”.
- Toute affirmation doit être prouvée par sortie de commande ou artefact Kaggle.
- Si un blocage survient, documenter immédiatement cause racine + correction appliquée + retest.

## Instruction d’exécution immédiate
Commence maintenant. N’interromps jamais le processus tant que le notebook de certification n’est pas validé sur Kaggle avec **zéro erreur et zéro warning**. Continue en boucle corrective jusqu’au succès complet, puis publie le rapport final et la traçabilité append-only.
