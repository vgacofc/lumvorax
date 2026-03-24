#ifndef EXACT_DIAGONALIZATION_H
#define EXACT_DIAGONALIZATION_H

/*
 * Exact Diagonalization (ED) Module — Cycle 17
 * Nouvel algorithme en parallèle de Monte Carlo pour cross-validation.
 *
 * Implémentations :
 *   ED_2x2  : Hubbard 2×2 demi-remplissage — diagonalisation exacte 36×36
 *             Espace de Fock : C(4,2)×C(4,2) = 6×6 = 36 états
 *             Énergie exacte : E0(U=4,t=1) ≈ −2.7279 eV
 *
 *   LANCZOS : Méthode de Lanczos pour systèmes jusqu'à 16 sites (N≤16)
 *             Convergence en ~50 itérations pour l'état fondamental
 *
 * Interface LumVorax : Toutes les métriques ED loggées via FORENSIC_LOG_ALGO
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Résultat ED ─────────────────────────────────────────────────── */
typedef struct {
    double   ground_energy_eV;    /* Énergie fondamentale exacte (eV) */
    double   first_excited_eV;    /* Premier état excité (eV) */
    double   gap_eV;              /* Gap spectral = E1 - E0 (eV) */
    double   double_occupancy;    /* <n↑n↓> au site 0 */
    double   pairing_corr;        /* Corrélation de pairing BCS <c†↑c†↓c↓c↑> */
    int      n_sites;             /* Nombre de sites simulés */
    int      hilbert_dim;         /* Dimension de l'espace de Hilbert utilisé */
    uint64_t elapsed_ns;          /* Durée de la diagonalisation (ns) */
    bool     converged;           /* Convergence atteinte */
    int      lanczos_iter;        /* Nombre d'itérations Lanczos (0 si ED exacte) */
} ed_result_t;

/* ── Paramètres Hubbard ──────────────────────────────────────────── */
typedef struct {
    int    lx, ly;       /* Dimensions du réseau */
    double t_eV;         /* Hopping en eV */
    double u_eV;         /* Répulsion Coulombienne en eV */
    double mu_eV;        /* Potentiel chimique en eV */
} ed_params_t;

/* ── API publique ───────────────────────────────────────────────── */

/* Diagonalisation exacte 2×2 Hubbard — O(36×36) */
ed_result_t ed_hubbard_2x2(const ed_params_t* p);

/* Diagonalisation exacte 4 sites 1D Hubbard — O(70×70) 
 * (C(4,2)^2 = 36 pour N=4, tous remplissages) */
ed_result_t ed_hubbard_1d4(const ed_params_t* p);

/* Méthode de Lanczos — N sites quelconques (N ≤ 16)
 * max_iter : nombre max d'itérations Lanczos (typiquement 50–200) */
ed_result_t ed_lanczos(const ed_params_t* p, int max_iter);

/* Comparaison ED vs Monte Carlo — retourne l'erreur relative */
double ed_compare_mc(const ed_result_t* ed, double mc_energy_eV,
                     const char* module_name);

/* Calcul analytique de l'énergie Bethe Ansatz 1D (Lieb-Wu 1968)
 * Valide pour chaîne 1D infinie à demi-remplissage */
double ed_bethe_ansatz_energy_1d(double u_eV, double t_eV, int n_points);

#endif /* EXACT_DIAGONALIZATION_H */
