# Cours intégral A→Z — Meta-neurones NX46-VX (V5 → V6 → future V7)

> Base d'analyse exigée: 
> - `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v5/nx46-vx-unified-kaggle-v5.ipynb`
> - `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v6/nx46-vx-unified-kaggle-v6.ipynb`
> - orientation future V7 via le pipeline `nx46-vesuvius-core-kaggle-ready-v7.7.py`

---

## 0) Pourquoi ce cours existe
Tu as demandé une explication **pédagogique complète** (anatomie, schémas, comportement, différences, rôle dans l'exécution, internet/offline, logs), centrée **strictement** sur V5 + V6 pour préparer V7.

Ce document répond exactement à cela: il explique ce qu'est le "meta-neurone" dans ce projet, où il agit, comment il décide, comment il échoue, et comment il doit évoluer en V7.

---

## 1) Définition précise du "meta-neurone" dans NX46-VX

Dans ce projet, un "meta-neurone" n'est pas un neurone biologique simulé au sens neurophysiologique pur.
C'est une **brique de pilotage méta** qui combine:
1. perception (features/gradients/signaux volumétriques),
2. décision (seuil, ratio, pruning/mutation),
3. gouvernance (traçabilité Merkle, logs forensics, conformité Kaggle).

Autrement dit: c'est un **cerveau de coordination de pipeline**.

---

## 2) Anatomie pédagogique (biologie ↔ code)

## 2.1 Schéma anatomique global

```text
             (Entrée sensorielle)
     Volume TIFF 3D + labels éventuels
                    |
                    v
        [DENDRITES] Acquisition + parsing
                    |
                    v
          [SOMA] Fusion de signaux
   (gradients Z/Y/X, contraste, score 3D/2.5D)
                    |
                    v
     [AXONE] Décision binaire de segmentation
     (seuil, ratio cible, 0/1 ou 0/255)
                    |
                    v
 [SYNAPSES / MÉMOIRE] Merkle + bit traces + metrics
                    |
                    v
        [MOTEUR] submission.zip + manifests
```

## 2.2 Schéma de contrôle (métacognition)

```text
meta_neurones
   ├─ observe: qualité signal / densité / stabilité
   ├─ ajuste: threshold / ratios / regularisation
   ├─ valide: format ZIP/TIFF + règles compétition
   └─ prouve : forensic logs + chaîne Merkle
```

---

## 3) Où le meta-neurone intervient dans le cycle d'exécution

## Étape A — Boot dépendances (offline-first)
- Les notebooks V5/V6 installent hors-ligne depuis `nx47-dependencies`.
- Objectif: éviter dépendance internet runtime Kaggle.

## Étape B — Préparation perception
- Chargement volumes 3D.
- Normalisation + extraction de signaux (gradients, features multi-échelle selon mode).

## Étape C — Cœur meta-neuronal décisionnel
- Stratégie `meta_neurons` + `ratio_candidates` + `pruning_quantile` + `mutation_noise`.
- Calibration de seuils (`threshold_scan`) et gestion stagnation F1.

## Étape D — Apprentissage / calibration
- Supervisé si labels exploitables.
- Fallback et calibrations adaptatives selon couverture train/validation.

## Étape E — Validation compétition
- Vérification binaire + forme TIFF + zip members.
- Publication `submission.zip` + aliases.

## Étape F — Forensic et preuve
- Logs événements, bit traces, Merkle chain, metrics, manifestes.

---

## 4) V5 vs V6: ce qui est identique, ce qui change réellement

## 4.1 Invariants majeurs V5/V6
Les deux versions conservent:
- architecture meta-neuronale (`meta_neurons=3`),
- logique évolutive (ratios/pruning/mutation),
- logique de calibration multi-seuil,
- mécanisme offline + forensic + Merkle,
- génération `submission.zip`.

## 4.2 Différences observées dans le code réel
La différence structurelle visible entre V5 et V6 (dans les notebooks fournis) est surtout:
1. un **runtime patch V6** injecté (politique GPU, sélection cible H100/P100, variables d'environnement strictes),
2. mise à jour des tags de version (`NX46-VX V6`, `nx46vx_v6`).

=> Important: V6 n'est pas une réécriture totale; c'est un renforcement opérationnel/runtime autour d'un noyau conceptuel quasi identique.

---

## 5) Lecture guidée du comportement meta-neuronal

## 5.1 Comportement normal attendu
- Le système lit le volume,
- construit un score probabiliste/énergétique,
- applique calibrage (seuil + ratio cible),
- exporte masque binaire conforme,
- trace toutes les étapes (audit complet).

## 5.2 Comportement en mode dégradé
- Si dataset absent: arrêt rapide (`dataset_root_required_but_not_found`).
- Si dépendance native incompatible: échec au chargement symbole `.so`.
- Si contenu zip invalide: rejet en mode strict compétition.

## 5.3 Interprétation "neuro" de ces échecs
- Ce ne sont pas des "bugs isolés", ce sont des ruptures d'homéostasie du meta-neurone:
  - perte d'entrée (dataset root),
  - synapse cassée (symboles natifs),
  - sortie motrice invalide (format compétition).

---

## 6) Processus internet vs offline (très clair)

## 6.1 Ce qui se passe **avec internet** (amont)
- Préparer le dataset de wheels.
- Versionner/pousser les artefacts.

## 6.2 Ce qui se passe **sans internet** (runtime Kaggle)
- Installer depuis dataset monté.
- Exécuter pipeline complet.
- Générer soumission + preuves.

Donc le meta-neurone exécute en offline-first; internet est surtout un **canal logistique de préparation**.

---

## 7) Comparaison avec approches existantes

## 7.1 vs U-Net pur end-to-end
- U-Net pur: optimisation directe pixels→masque.
- Meta-neurone NX46-VX: surcouche de pilotage (décision + conformité + forensic) au-dessus du calcul.

## 7.2 vs heuristique brute
- Heuristique brute: simple mais peu traçable.
- Meta-neurone: ajoute gouvernance, auditabilité, non-régression format.

## 7.3 vs pipeline C natif strict
- Natif strict: performances potentielles élevées mais fragilité d'intégration Kaggle.
- V7 visé: Python pur robuste, contrôlé, reproductible.

---

## 8) Pont vers future V7 (à partir V5/V6)

L'évolution logique V7 est:
1. garder le noyau meta-neuronal V5/V6,
2. simplifier l'intégration runtime,
3. renforcer la validation sortie,
4. augmenter la qualité de scoring 3D,
5. conserver traçabilité complète.

La version v7.7 actuelle du repo illustre ce cap:
- scoring 3D natif + blending,
- validation stricte du contenu zip,
- exports de manifests dépendances,
- stratégie fallback supervisé/non supervisé plus explicite.

---

## 9) Graphiques pédagogiques (ASCII)

## 9.1 Graphe de flux décisionnel

```text
Input Volume
    |
    +--> Signal Builder -----> Score Map -----> Threshold Engine -----> Binary Mask
    |                               |                    |                    |
    |                               v                    v                    v
    +------------------------> Meta-Neuron Control <----+--------> Compliance Gate
                                            |
                                            v
                                      Forensic Ledger
```

## 9.2 Courbe conceptuelle densité vs rappel

```text
Rappel
1.0 |                * zone sur-prédiction
    |             *
0.7 |          *
    |       *
0.5 |    *          <- zone cible (équilibre)
    |  *
0.2 |*                      zone sous-prédiction
    +----------------------------------------------
      faible densité              forte densité
```

Lecture: un meta-neurone mal calibré peut être trop conservateur (sous-segmentation), ce qui dégrade le score même si le pipeline est techniquement propre.

---

## 10) Checklist de compréhension totale (cours)
Si tu comprends ces 10 points, tu maîtrises le système:
1. Rôle exact du meta-neurone (pilotage méta, pas neurone biologique pur).
2. Anatomie fonctionnelle (perception → décision → gouvernance).
3. Place dans l'exécution runtime.
4. Invariants V5/V6.
5. Différences V5→V6 réellement observables.
6. Modes d'échec et leur sens systémique.
7. Frontière internet/offline.
8. Différences avec U-Net pur / heuristique brute / C natif.
9. Orientation technique V7.
10. Lien entre calibration densité et score final.

---

## 11) Conclusion "cours magistral"
Le meta-neurone NX46-VX est le **chef d'orchestre** du pipeline: il ne "fait" pas seulement la segmentation, il garantit que la segmentation est:
- calculée,
- calibrée,
- validée,
- traçable,
- publiable.

Sans lui, tu as au mieux un modèle qui prédit; avec lui, tu as un système exploitable en compétition réelle.
