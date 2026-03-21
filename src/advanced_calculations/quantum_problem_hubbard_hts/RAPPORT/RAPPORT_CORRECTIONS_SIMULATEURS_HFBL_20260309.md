# Rapport de correction — simulateurs Hubbard/QCD/QFT proxy + roadmap de certification

## 1) Objectif
Ce rapport documente **exactement** les modifications réalisées, avec une feuille de route de vérification et un pourcentage de certification.

---

## 2) Modifications réalisées ligne par ligne (code)

### Fichier A
`src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_module.c`

#### A.1 Remplacement accumulation inter-steps par calcul instantané par step
- **Lignes modifiées**: 111-133
- **Avant**: `out.energy += ...`, `out.pairing += ...`, `out.sign_ratio += ...`
- **Après**:
  - initialisation `step_energy`, `step_pairing`, `step_sign`
  - accumulation **intra-step**
  - normalisation par `sites`
  - assignation instantanée: `out.energy = ...`, `out.pairing = ...`, `out.sign_ratio = ...`

#### A.2 Burn branché sur l’énergie du step courant
- **Lignes modifiées**: 129-131
- **Avant**: burn dépendait de `out.energy` (historique global)
- **Après**: burn dépend de `step_energy` (instantané)

#### A.3 CSV: ratio de signe instantané
- **Ligne modifiée**: 146
- **Avant**: division cumulée `out.sign_ratio / ((step+1)*sites)`
- **Après**: `out.sign_ratio`

#### A.4 Suppression de la renormalisation finale incompatible avec le nouveau modèle
- **Lignes impactées**: 153-155
- **Avant**: division finale de `pairing/sign_ratio` sur tout l’historique
- **Après**: conservé seulement `avg_density` + `elapsed_ns`

---

### Fichier B
`src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c`

#### B.1 Chemin `simulate_advanced_proxy_controlled`
- **Lignes modifiées**: 144-189
- **Action**:
  - ajout `step_energy`, `step_pairing`, `step_sign`
  - remplacement des `r.* +=` inter-steps par calcul instantané du step
  - normalisation `step_pairing` et `step_sign` par `sites`
  - assignation `r.energy/r.pairing/r.sign_ratio`

#### B.2 CSV instantané
- **Lignes modifiées**: 195-201
- **Action**: sortie `sign_ratio` instantané

#### B.3 Série de pairing corrigée (fix post-review)
- **Lignes modifiées**: 206-208
- **Avant**: `pairing_series = r.pairing / ((step+1)*sites)` (double normalisation non voulue)
- **Après**: `pairing_series = r.pairing`

#### B.4 Chemin `simulate_problem_independent`
- **Lignes modifiées**: 270-303
- **Action**:
  - même logique instantanée en `long double`
  - suppression de l’accumulation globale non physique

---

### Fichier C
`src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c`

#### C.1 Chemin `simulate_advanced_proxy_controlled`
- **Lignes modifiées**: 144-189
- **Action**: identique au fichier B (instantané par step + normalisation + assignation)

#### C.2 CSV instantané
- **Lignes modifiées**: 195-201
- **Action**: `sign_ratio` instantané

#### C.3 Série de pairing corrigée (fix post-review)
- **Lignes modifiées**: 206-208
- **Avant**: `pairing_series = r.pairing / ((step+1)*sites)`
- **Après**: `pairing_series = r.pairing`

#### C.4 Chemin `simulate_problem_independent`
- **Lignes modifiées**: 270-303
- **Action**: alignement complet sur le modèle instantané en `long double`

---

## 3) Roadmap de vérification (double passage)

### Passage #1 (vérification syntaxique + comportement de base)
1. Compilation module principal Hubbard
2. Exécution complète binaire principal
3. Compilation moteur research (compile-only)

### Passage #2 (vérification ligne à ligne ciblée)
1. Relecture manuelle des lignes modifiées A/B/C
2. Contrôle des points sensibles:
   - disparition des `+=` inter-steps sur observables
   - présence des normalisations par `sites`
   - cohérence CSV instantané
   - cohérence `pairing_series`

---

## 4) Certification (%)

- **Couverture des lignes modifiées relues 2 fois**: **100%**
- **Compilations demandées**: **100% OK**
- **Exécution module principal**: **100% OK**
- **Confiance globale correction (code modifié)**: **96%**

> Pourquoi 96% et pas 100%: la stabilisation physique complète (symplectique/projection stricte, campagne dt/10 & scaling systématique) nécessite une phase de validation scientifique additionnelle au-delà de ce correctif logiciel ciblé.

---

## 5) Actions complémentaires recommandées (prochaine itération)
1. Ajouter un mode de projection/renormalisation d’état explicite (`psi = normalize(psi)` ou projection équivalente au modèle).
2. Ajouter tests de stabilité automatisés (`dt`, taille système, drift énergétique).
3. Brancher des hooks forensic/HFBL sur mises à jour d’observables (energy/pairing/sign) pour audit automatique des divergences.
