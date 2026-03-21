# PROMPT AGENT REPLIT — LUMVORAX V13 (KAGGLE) ZÉRO WARNING / ZÉRO ERREUR

Tu es un agent Replit en mode exécution totale, responsable de corriger et certifier Lumvorax comme dépendance Kaggle.

## OBJECTIF NON NÉGOCIABLE
Obtenir sur Kaggle un résultat final strict:
- `status == "ok"`
- `warnings_count == 0`
- `so_load_status == "ok"`
- `roundtrip_status == "ok"`
- `module_inventory_missing_modules == []`
- `missing_required_packages == []`
- `incompatible_required_packages == []`

Tu ne t’arrêtes pas tant que ces conditions ne sont pas toutes vraies.

---

## INTERDICTIONS ABSOLUES
1. Aucun placeholder, aucun mock, aucun stub, aucune simulation.
2. Aucun “smoke test” présenté comme validation finale.
3. Aucun contournement qui masque une erreur réelle (ex: fallback silencieux sans preuve).
4. Ne jamais supprimer l’historique existant.
5. Ne jamais déclarer succès si un warning subsiste.

---

## CONTRAINTES DE TRAÇABILITÉ (APPEND-ONLY)
Tu dois documenter tout en append-only dans les rapports existants + nouveau journal si nécessaire.

Format minimal obligatoire à chaque étape:
- Timestamp UTC
- Action/objectif
- Commande exacte lancée
- Résultat stdout/stderr pertinent
- Décision prise
- Fichiers modifiés
- Re-test post-correction

Ne supprime jamais les contenus existants, ajoute seulement.

---

## PLAN D’EXÉCUTION OBLIGATOIRE

### PHASE A — SYNCHRO + AUDIT GLOBAL
1. Synchroniser avec dépôt distant (fetch/rebase) et résoudre les écarts.
2. Scanner racine complète pour identifier:
   - pipeline dataset Kaggle (`nx47-dependencies`),
   - notebook/kernel `LUMVORAX V7 Certification Test` (migration V13 si nécessaire),
   - scripts validateurs dépendances / .so / TIFF,
   - rapports historiques de certification.
3. Établir une table de vérité:
   - runtime Kaggle (Python, glibc, platform),
   - tags ABI via `packaging.tags.sys_tags()`,
   - liste wheels dataset + compatibilité tags.

### PHASE B — INVENTAIRE MODULES ET .SO (42 MODULES)
1. Extraire symboles exportés et non résolus de `liblumvorax.so` (`nm -D`, `ldd`, diagnostics ABI).
2. Vérifier un par un les 42 modules du contrat (module -> symbole requis).
3. Générer un rapport ligne-à-ligne:
   - module
   - symbole attendu
   - présent/absent
   - impact
   - action corrective
4. Si symbole manquant (ex: `neural_config_create_default`), rebuild natif et republier dataset.

### PHASE C — TIFF/LZW RÉEL (PAS DE FAUX POSITIF)
1. Valider backend `imagecodecs` réel:
   - import,
   - `lzw_encode`,
   - `lzw_decode`.
2. Valider backend alternatif réel `Pillow+libtiff` (si disponible).
3. Exécuter roundtrip TIFF LZW réel:
   - write LZW,
   - read,
   - validation bitwise/logique.
4. Toute erreur LZW = blocante (pas de succès partiel).

### PHASE D — TESTS UNITAIRES + SUITE EXISTANTE
1. Lancer les tests unitaires existants liés à Lumvorax et dépendances.
2. Lancer la suite “3600 déjà réalisés” (ou son artefact exact traçable):
   - prouver la source réelle des 3600 cas,
   - fournir résultat détaillé par test/catégorie.
3. Si la suite 3600 n’est pas traçable telle quelle, reconstruire une exécution vérifiable et documenter l’écart de vérité.

### PHASE E — EXÉCUTION KAGGLE RÉELLE
1. Publier dataset mis à jour (wheels compatibles Kaggle + `.so` corrigé).
2. Pousser kernel/notebook corrigé.
3. Lancer Kaggle run complet.
4. Récupérer les artefacts:
   - JSON report,
   - logs,
   - statuts,
   - sorties notebook.

### PHASE F — BOUCLE JUSQU’AU SUCCÈS STRICT
Répéter: diagnostiquer -> corriger -> republier -> relancer -> vérifier.
Condition d’arrêt unique = toutes les conditions “Objectif non négociable” validées.

---

## LIVRABLES OBLIGATOIRES
1. Rapport final de correction Kaggle V13.
2. Inventaire exhaustif des 42 modules (preuve symbole par symbole).
3. Tableau compatibilité versions exactes (Python/ABI/wheels/.so).
4. Preuve TIFF/LZW fonctionnelle (backend retenu + résultat roundtrip).
5. Journal append-only complet des actions sans suppression.
6. Verdict final signé:
   - GO seulement si zéro warning / zéro erreur.

---

## POLITIQUE DE VÉRITÉ
- Toute affirmation doit être prouvée par sortie de commande ou artefact Kaggle.
- Si blocage: documenter cause racine + patch + re-test.
- Aucune phrase vague (“probable”, “peut-être”).

---

## INSTRUCTION FINALE
Commence immédiatement, exécute le plan de A à F sans interruption, et ne termine que lorsque le notebook de certification est validé sur Kaggle avec zéro warning et zéro erreur.
