/**
 * worm_mc_bosonic.h — Algorithme Worm Monte Carlo bosonique (C36-P3)
 *
 * Implémente l'algorithme de Prokof'ev–Svistunov pour systèmes bosoniques
 * sur réseau 2D (Hubbard bosonique, modèle Bose-Hubbard).
 *
 * Référence : Prokof'ev, Svistunov & Tupitsyn, Phys. Lett. A 238, 253 (1998)
 *
 * Namespace log : FORENSIC_LOG_MODULE_METRIC (jamais QF_).
 */

#ifndef WORM_MC_BOSONIC_H
#define WORM_MC_BOSONIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Paramètres Bose-Hubbard
 * ------------------------------------------------------------------------- */

#define WORM_MC_MAX_SITES 256      /* taille max du réseau (Lx*Ly) */
#define WORM_MC_MAX_STEPS 100000   /* sweeps MC max                */

typedef struct {
    int    Lx;            /* largeur du réseau 2D                */
    int    Ly;            /* hauteur du réseau 2D                */
    double t;             /* hopping bosonique [eV]              */
    double U;             /* interaction on-site [eV]            */
    double mu;            /* potentiel chimique [eV]             */
    double beta;          /* inverse température β = 1/(kB T)   */
    uint64_t seed;        /* graine RNG LCG                      */
    int    n_sweeps;      /* nombre de sweeps MC demandés        */
    int    n_warmup;      /* sweeps de thermalisation            */
} worm_mc_params_t;

/* -------------------------------------------------------------------------
 * État du réseau (configuration de worldlines)
 * ------------------------------------------------------------------------- */

typedef struct {
    int    n[WORM_MC_MAX_SITES];       /* occupation de chaque site (bosons) */
    int    n_sites;                    /* nombre total de sites              */
    double E_total;                    /* énergie totale estimée [eV]        */
    double n_avg;                      /* occupation moyenne par site        */
    double n2_avg;                     /* <n²> moyen (fluctuations)          */
    double compressibility;            /* κ = β(<N²> - <N>²) / N_sites      */
    long   n_worm_accepted;            /* insertions de ver acceptées        */
    long   n_worm_proposed;            /* insertions de ver proposées        */
    bool   superfluid;                 /* phase superfluide détectée         */
    double superfluid_density;         /* densité superfluide ρ_s            */
} worm_mc_state_t;

/* -------------------------------------------------------------------------
 * Résultats d'une campagne MC
 * ------------------------------------------------------------------------- */

typedef struct {
    double E_per_site;       /* énergie par site [eV]         */
    double n_per_site;       /* bosons par site               */
    double compressibility;  /* compressibilité κ             */
    double superfluid_density; /* densité superfluide ρ_s     */
    double acceptance_rate;  /* taux d'acceptation ver        */
    bool   superfluid;       /* phase superfluide ?           */
    bool   converged;        /* convergence MC atteinte ?     */
    int    n_sweeps_done;    /* sweeps réellement effectués   */
} worm_mc_result_t;

/* -------------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------------- */

/**
 * worm_mc_init — Initialise l'état MC à partir des paramètres.
 * Remplit state avec la configuration initiale (vide) et initialise le RNG.
 */
void worm_mc_init(const worm_mc_params_t *params, worm_mc_state_t *state);

/**
 * worm_mc_run — Exécute les sweeps MC et retourne les observables.
 *
 * Algorithme :
 *   1. Insertion/suppression de ver (Ira+Masha)
 *   2. Déplacement de tête de ver (hopping)
 *   3. Mesure des observables (<n>, <n²>, ρ_s) après thermalisation
 *
 * @return true si convergence atteinte, false sinon.
 */
bool worm_mc_run(const worm_mc_params_t *params, worm_mc_state_t *state,
                 worm_mc_result_t *result);

/**
 * worm_mc_write_csv — Écrit les observables dans un fichier CSV.
 * Colonnes : module,observable,T,U,value,status
 */
int worm_mc_write_csv(const worm_mc_params_t *params,
                      const worm_mc_result_t *result,
                      FILE *csv_out);

/**
 * worm_mc_classify_phase — Détermine la phase (superfluide / Mott / normal).
 * Retourne une chaîne statique : "superfluid", "mott_insulator", "normal".
 */
const char *worm_mc_classify_phase(const worm_mc_result_t *result);

/**
 * worm_mc_set_log_files — Configure les CSV forensiques ultra-profonds.
 * G-C39-WORM-ULTRA : Activer avant worm_mc_run(), désactiver après.
 *   sweep_log  : un enregistrement par sweep (acceptance_rate, beta_eff, etc.)
 *   attempt_log: un enregistrement par tentative Metropolis individuelle
 * Passer NULL pour désactiver le logging.
 */
void worm_mc_set_log_files(FILE* sweep_log, FILE* attempt_log);

#ifdef __cplusplus
}
#endif

#endif /* WORM_MC_BOSONIC_H */
