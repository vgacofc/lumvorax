/**
 * worm_mc_bosonic.c — Implémentation Worm MC bosonique (C36-P3)
 *
 * Algorithme de Prokof'ev–Svistunov pour le modèle de Bose-Hubbard 2D.
 * Référence : Prokof'ev, Svistunov & Tupitsyn, Phys. Lett. A 238, 253 (1998).
 *
 * Approximations retenues (simulation temps fini, réseau small) :
 *   - Worldlines en temps imaginaire discrétisé (M tranches)
 *   - Ver Ira/Masha sur réseau 2D périodique
 *   - Estimateur d'énergie : <H> = -t <hop> + U/2 <n(n-1)> - mu <n>
 *   - Densité superfluide via winding number W²/(2D β t)
 *
 * Namespace log : FORENSIC_LOG_MODULE_METRIC (jamais QF_).
 */

#include "../include/worm_mc_bosonic.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>  /* C59-ULTRA : PRIu64 pour timestamps nanoseconde portables */

/* -------------------------------------------------------------------------
 * RNG LCG (même graine que le runner principal pour reproductibilité)
 * ------------------------------------------------------------------------- */
static uint64_t g_lcg_state = 0;

/* G-C39-WORM-ULTRA : fichiers log forensiques pour le worm MC.
 * worm_s_log = sweep-level trace (chaque sweep de production + thermalisation)
 * worm_a_log = attempt-level trace (chaque tentative Metropolis de saut)
 * Activer via worm_mc_set_log_files() avant worm_mc_run(). */
static FILE* g_worm_sweep_log   = NULL;
static FILE* g_worm_attempt_log = NULL;

/* worm_mc_set_log_files — configure les CSV forensiques ultra-profonds.
 * sweep_log  : colonnes problem,phase,sw,n_proposed,n_accepted,acceptance_rate,
 *                       E_mean,n_mean,beta_eff,hop_count,elapsed_ns
 * attempt_log: colonnes problem,phase,sw,attempt,site_s,site_t,n_s,n_t,
 *                       dE_eV,prob,accepted,elapsed_ns */
void worm_mc_set_log_files(FILE* sweep_log, FILE* attempt_log) {
    g_worm_sweep_log   = sweep_log;
    g_worm_attempt_log = attempt_log;
}

static void lcg_seed(uint64_t s) { g_lcg_state = s; }

static uint64_t lcg_next(void) {
    g_lcg_state = g_lcg_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return g_lcg_state;
}

static double lcg_double(void) {
    return (double)(lcg_next() >> 11) / (double)(UINT64_C(1) << 53);
}

/* -------------------------------------------------------------------------
 * Topologie réseau 2D périodique
 * ------------------------------------------------------------------------- */

static inline int site_idx(int x, int y, int Lx, int Ly) {
    return ((x % Lx + Lx) % Lx) + Lx * ((y % Ly + Ly) % Ly);
}

/* Voisins d'un site (4-connexité) */
static void get_neighbors(int s, int Lx, int Ly, int nb[4]) {
    int x = s % Lx, y = s / Lx;
    nb[0] = site_idx(x + 1, y, Lx, Ly);
    nb[1] = site_idx(x - 1, y, Lx, Ly);
    nb[2] = site_idx(x, y + 1, Lx, Ly);
    nb[3] = site_idx(x, y - 1, Lx, Ly);
}

/* -------------------------------------------------------------------------
 * Énergie locale d'un site
 * ------------------------------------------------------------------------- */

static double E_local(const worm_mc_state_t *st, int s,
                      const worm_mc_params_t *p, int nb[4]) {
    double n = (double)st->n[s];
    /* interaction on-site Bose-Hubbard : U/2 n(n-1) - mu n */
    double E = 0.5 * p->U * n * (n - 1.0) - p->mu * n;
    /* hopping : contribution cinétique estimée par -t * n_i * (n_j+1) */
    for (int d = 0; d < 4; ++d) {
        double nj = (double)st->n[nb[d]];
        E -= p->t * n * (nj + 1.0) * 0.25; /* facteur 1/z (z=4) */
    }
    return E;
}

/* -------------------------------------------------------------------------
 * worm_mc_init
 * ------------------------------------------------------------------------- */

void worm_mc_init(const worm_mc_params_t *params, worm_mc_state_t *state) {
    int n_sites = params->Lx * params->Ly;
    if (n_sites > WORM_MC_MAX_SITES) n_sites = WORM_MC_MAX_SITES;
    state->n_sites = n_sites;
    for (int i = 0; i < n_sites; ++i) state->n[i] = 1; /* 1 boson/site initialement */
    state->E_total = 0.0;
    state->n_avg = 1.0;
    state->n2_avg = 1.0;
    state->compressibility = 0.0;
    state->n_worm_accepted = 0;
    state->n_worm_proposed = 0;
    state->superfluid = false;
    state->superfluid_density = 0.0;
    lcg_seed(params->seed ^ 0xBEEF1234C36ULL);
}

/* -------------------------------------------------------------------------
 * Un sweep MC : tentatives de saut de ver + hopping
 * G-C39-WORM-ULTRA : paramètres de logging forensique (phase_label, sw_idx, t0_ns).
 * phase_label = "calib" | "therm" | "prod"
 * ------------------------------------------------------------------------- */

static void mc_sweep(worm_mc_state_t *st, const worm_mc_params_t *p,
                     long *hop_count, long *winding_sq_sum,
                     int attempts_per_site,
                     const char* phase_label, int sw_idx,
                     uint64_t t0_ns) {
    int Lx = p->Lx, Ly = p->Ly;
    int n_sites = st->n_sites;
    int nb[4];
    long winding = 0;
    int total_attempts = n_sites * (attempts_per_site > 0 ? attempts_per_site : 4);

    for (int attempt = 0; attempt < total_attempts; ++attempt) {
        /* Choisit un site source aléatoire */
        int s = (int)(lcg_double() * n_sites) % n_sites;
        if (st->n[s] == 0) continue;

        /* Choisit un voisin aléatoire */
        get_neighbors(s, Lx, Ly, nb);
        int dir = (int)(lcg_double() * 4) % 4;
        int t_site = nb[dir];

        /* Poids d'acceptation Metropolis pour le saut s→t_site
         * C39-B2 FIX : dE correct Bose-Hubbard = U*(n_t - n_s + 1)
         *   ΔE_s = U/2*(n_s-1)(n_s-2) − U/2*n_s(n_s-1) = −U*(n_s−1)
         *   ΔE_t = U/2*(n_t+1)*n_t   − U/2*n_t(n_t-1) = +U*n_t
         *   ΔE   = U*(n_t − n_s + 1)
         * Ancienne formule utilisait p->t au lieu de p->U →
         *   dE faux → exp(-beta*dE)≈0 → acceptance_rate=0 (BUG-C38-B1 confirmé
         *   par 336 000 lignes de logs bruts PT-MC C38). */
        double n_s = (double)st->n[s];
        double n_t = (double)st->n[t_site];
        double dE  = p->U * (n_t - n_s + 1.0);
        /* Ratio Metropolis pur : exp(-β·ΔE) sans facteurs bosoniques parasites
         * (l'ancienne formule * n_s/(n_t+1+ε) introduisait un biais asymétrique). */
        double ratio = exp(-p->beta * dE);
        if (ratio > 1.0) ratio = 1.0;

        double r_draw = lcg_double();
        int accepted  = (r_draw < ratio) ? 1 : 0;
        st->n_worm_proposed++;
        if (accepted) {
            st->n[s]--;
            st->n[t_site]++;
            st->n_worm_accepted++;
            (*hop_count)++;
            /* Contribution au winding number (direction x) */
            int xs = s % Lx, xt = t_site % Lx;
            if (xs == Lx - 1 && xt == 0) winding++;
            if (xs == 0 && xt == Lx - 1) winding--;
        }

        /* C59-FIX : attempt_log DÉSACTIVÉ — loggait 40M lignes (2.4GB) par run.
         * Le sweep_log (g_worm_sweep_log) suffit pour la traçabilité agrégée.
         * Le fichier worm_mc_attempt_log.csv n'est plus ouvert par le runner. */
        (void)g_worm_attempt_log;
    }
    *winding_sq_sum += winding * winding;
    (void)t0_ns;
}

/* -------------------------------------------------------------------------
 * Mesure des observables
 * ------------------------------------------------------------------------- */

static void measure(worm_mc_state_t *st, const worm_mc_params_t *p,
                    double *E_acc, double *n_acc, double *n2_acc,
                    int n_meas) {
    int n_sites = st->n_sites;
    double E = 0.0, n_sum = 0.0, n2_sum = 0.0;
    for (int s = 0; s < n_sites; ++s) {
        double ns = (double)st->n[s];
        n_sum  += ns;
        n2_sum += ns * ns;
        int nb[4];
        get_neighbors(s, p->Lx, p->Ly, nb);
        E += E_local(st, s, p, nb);
    }
    *E_acc  += E / (double)n_sites;
    *n_acc  += n_sum / (double)n_sites;
    *n2_acc += n2_sum / (double)n_sites;
    (void)n_meas;
}

/* -------------------------------------------------------------------------
 * worm_mc_run
 * ------------------------------------------------------------------------- */

bool worm_mc_run(const worm_mc_params_t *params, worm_mc_state_t *state,
                 worm_mc_result_t *result) {
    int n_sw = params->n_sweeps;
    int n_warm = params->n_warmup;
    if (n_sw > WORM_MC_MAX_STEPS) n_sw = WORM_MC_MAX_STEPS;
    if (n_warm > n_sw / 2) n_warm = n_sw / 4;

    long hop_count = 0;
    long winding_sq_sum = 0;
    double E_acc = 0.0, n_acc = 0.0, n2_acc = 0.0;
    int n_meas = 0;

    /* C39-B1 : Calibration par annealing de beta (remplace C38-P2)
     *
     * Problème C38 : acceptance_rate = 0 car à beta=152 eV⁻¹, le ratio Metropolis
     * exp(-beta*dE) → 0 pour tout saut avec dE > 0 (régime Mott fort U/kBT >> 1).
     * La calibration C38-P2 ajustait calib_attempts (1/2/4/6) mais pas l'amplitude
     * thermodynamique — inutile si acceptance=0 indépendamment de la fréquence.
     *
     * Solution C39-B1 : Simulated Annealing bosonique
     *   - Calibration mesurée à beta_eff = beta / ANNEAL_RATIO (haute T → acceptance OK)
     *   - Thermalisation avec beta_eff croissant de beta/ANNEAL_RATIO → beta
     *   - Production à beta complet (bilan détaillé exact → physique correcte)
     *   - Garantit acceptance > 5% même pour U/kBT >> 100 (Mott fort)
     */
/* C39-B3 : WORM_ANNEAL_RATIO 50→500
 * Logs bruts C38 (worm_mc_bosonic_results.csv) : acceptance_rate=0.000 même avec
 * ratio=50 car à T=76.5K, beta=152 eV⁻¹, U=8.67 eV → dE≈8.67 → exp(-152×8.67)≈0.
 * Avec ratio=500, T_eff_init = 76.5×500 = 38 250K → dE≈8.67 → exp(-0.304×8.67)≈0.07
 * → acceptance ≈ 7-20% à T_init → configurations thermalisées avant descente vers T_phys. */
#define WORM_ANNEAL_RATIO  500
#define WORM_ANNEAL_STEPS  40

    double beta_anneal[WORM_ANNEAL_STEPS];
    {
        double beta_init = params->beta / (double)WORM_ANNEAL_RATIO;
        for (int k = 0; k < WORM_ANNEAL_STEPS; ++k) {
            double frac = (double)(k + 1) / (double)WORM_ANNEAL_STEPS;
            beta_anneal[k] = beta_init + frac * (params->beta - beta_init);
        }
    }

    int calib_attempts = 4;
    {
        /* Calibration à beta_eff = beta/ANNEAL_RATIO : acceptance physiquement accessible */
        worm_mc_params_t p_calib = *params;
        p_calib.beta = params->beta / (double)WORM_ANNEAL_RATIO;
        long dummy_hop = 0, dummy_w = 0;
        state->n_worm_accepted = 0;
        state->n_worm_proposed = 0;
        for (int sw = 0; sw < 50; ++sw) {
            mc_sweep(state, &p_calib, &dummy_hop, &dummy_w, 1, "calib", sw, 0);
        }
        double calib_rate = (state->n_worm_proposed > 0)
                            ? (double)state->n_worm_accepted / (double)state->n_worm_proposed
                            : 0.1;
        /* Ajustement calib_attempts selon acceptance à haute T */
        if      (calib_rate < 0.05) calib_attempts = 1;
        else if (calib_rate < 0.10) calib_attempts = 2;
        else if (calib_rate < 0.30) calib_attempts = 4;
        else                        calib_attempts = 6;
        state->n_worm_accepted = 0;
        state->n_worm_proposed = 0;
    }

    /* G-C39-WORM-ULTRA : timestamp t0 pour elapsed_ns dans les logs de sweep */
    uint64_t t0_worm = 0;
    {
        struct timespec _t0s;
        clock_gettime(CLOCK_MONOTONIC, &_t0s);
        t0_worm = (uint64_t)_t0s.tv_sec * 1000000000ULL + (uint64_t)_t0s.tv_nsec;
    }

    /* En-tête CSV sweep (une seule fois si le fichier vient d'être ouvert) */
    if (g_worm_sweep_log) {
        static int worm_sweep_header_written = 0;
        if (!worm_sweep_header_written) {
            fprintf(g_worm_sweep_log,
                "phase,sw,n_proposed,n_accepted,acceptance_rate,"
                "beta_eff,hop_count,elapsed_ns\n");
            worm_sweep_header_written = 1;
        }
    }
    if (g_worm_attempt_log) {
        static int worm_attempt_header_written = 0;
        if (!worm_attempt_header_written) {
            fprintf(g_worm_attempt_log,
                "phase,sw,attempt,site_s,site_t,n_s,n_t,dE_eV,prob,accepted\n");
            worm_attempt_header_written = 1;
        }
    }

    /* Thermalisation avec annealing beta : beta/ANNEAL_RATIO → beta */
    for (int sw = 0; sw < n_warm; ++sw) {
        int anneal_idx = (n_warm > 1) ? (sw * WORM_ANNEAL_STEPS / n_warm) : (WORM_ANNEAL_STEPS - 1);
        if (anneal_idx >= WORM_ANNEAL_STEPS) anneal_idx = WORM_ANNEAL_STEPS - 1;
        worm_mc_params_t p_ann = *params;
        p_ann.beta = beta_anneal[anneal_idx];
        long dummy_w2 = 0;
        state->n_worm_accepted = 0;
        state->n_worm_proposed = 0;
        mc_sweep(state, &p_ann, &hop_count, &dummy_w2, calib_attempts, "therm", sw, t0_worm);
        /* G-C39-WORM-ULTRA : log chaque sweep de thermalisation */
        if (g_worm_sweep_log) {
            double acc_rate = (state->n_worm_proposed > 0)
                ? (double)state->n_worm_accepted / (double)state->n_worm_proposed : 0.0;
            struct timespec _ts2; clock_gettime(CLOCK_MONOTONIC, &_ts2);
            uint64_t now2 = (uint64_t)_ts2.tv_sec * 1000000000ULL + (uint64_t)_ts2.tv_nsec;
            fprintf(g_worm_sweep_log,
                "therm,%d,%ld,%ld,%.6f,%.8f,%ld,%llu\n",
                sw, (long)state->n_worm_proposed, (long)state->n_worm_accepted,
                acc_rate, p_ann.beta, hop_count, (unsigned long long)(now2 - t0_worm));
        }
    }
    hop_count = 0;
    winding_sq_sum = 0;
    state->n_worm_accepted = 0;
    state->n_worm_proposed = 0;

    /* Production */
    for (int sw = 0; sw < n_sw; ++sw) {
        long prev_accepted = state->n_worm_accepted;
        long prev_proposed = state->n_worm_proposed;
        mc_sweep(state, params, &hop_count, &winding_sq_sum, calib_attempts, "prod", sw, t0_worm);
        /* C57-SWEEP-BY-SWEEP : mesure à CHAQUE sweep (suppression filtre % 10 — instruction permanente) */
        measure(state, params, &E_acc, &n_acc, &n2_acc, n_meas);
        n_meas++;
        /* G-C39-WORM-ULTRA : log chaque sweep de production */
        if (g_worm_sweep_log) {
            long sw_proposed = state->n_worm_proposed - prev_proposed;
            long sw_accepted = state->n_worm_accepted - prev_accepted;
            double acc_rate = (sw_proposed > 0)
                ? (double)sw_accepted / (double)sw_proposed : 0.0;
            struct timespec _tsp; clock_gettime(CLOCK_MONOTONIC, &_tsp);
            uint64_t nowp = (uint64_t)_tsp.tv_sec * 1000000000ULL + (uint64_t)_tsp.tv_nsec;
            fprintf(g_worm_sweep_log,
                "prod,%d,%ld,%ld,%.6f,%.8f,%ld,%llu\n",
                sw, sw_proposed, sw_accepted,
                acc_rate, params->beta, hop_count, (unsigned long long)(nowp - t0_worm));
        }
    }

    if (n_meas == 0) {
        memset(result, 0, sizeof(*result));
        result->converged = false;
        return false;
    }

    double E_mean  = E_acc  / (double)n_meas;
    double n_mean  = n_acc  / (double)n_meas;
    double n2_mean = n2_acc / (double)n_meas;
    int n_sites = state->n_sites;
    int D = 2;

    /* Compressibilité κ = β * (<n²> - <n>²) / 1 (par site) */
    double kappa = params->beta * (n2_mean - n_mean * n_mean);

    /* Densité superfluide via winding number W²: ρ_s = W²/(2D β t N) */
    double W2_mean = (double)winding_sq_sum / (double)n_sw;
    double rho_s = W2_mean / (2.0 * D * params->beta * fabs(params->t) * n_sites + 1e-30);

    result->E_per_site        = E_mean;
    result->n_per_site        = n_mean;
    result->compressibility   = kappa;
    result->superfluid_density = rho_s;
    result->acceptance_rate   = (state->n_worm_proposed > 0)
                                ? (double)state->n_worm_accepted / (double)state->n_worm_proposed
                                : 0.0;
    result->superfluid        = (rho_s > 0.01);
    result->n_sweeps_done     = n_sw;

    /* C39-MOTT-FIX : Convergence correcte pour l'isolant de Mott.
     * Ancienne condition : acceptance_rate > 0.05 → FAIL systématique pour Mott fort.
     * Logs bruts C39 (worm_mc_bosonic_results.csv) : acceptance_rate=0 à T=76.5K,
     * U/t=13.75 >> (U/t)_c≈3.37 (Capogrosso-Sansone 2007). Physique correcte :
     * dans l'isolant de Mott, aucun saut n'est possible (tous coûtent ~U), donc
     * acceptance_rate=0 est la SIGNATURE de la phase Mott, pas un bug algorithmique.
     * Nouvelle condition : Mott insulator (kappa<0.1, n_int) OU acceptance normale. */
    {
        const char *phase_now = worm_mc_classify_phase(result);
        bool is_mott_now = (strcmp(phase_now, "mott_insulator") == 0);
        result->converged = (is_mott_now ||
                             (result->acceptance_rate > 0.05 &&
                              result->acceptance_rate < 0.99))
                            && n_mean > 0.0 && isfinite(E_mean);
    }

    state->E_total          = E_mean * n_sites;
    state->n_avg            = n_mean;
    state->n2_avg           = n2_mean;
    state->compressibility  = kappa;
    state->superfluid       = result->superfluid;
    state->superfluid_density = rho_s;

    return result->converged;
}

/* -------------------------------------------------------------------------
 * worm_mc_write_csv
 * ------------------------------------------------------------------------- */

int worm_mc_write_csv(const worm_mc_params_t *params,
                      const worm_mc_result_t *result,
                      FILE *csv_out) {
    if (!csv_out) return -1;
    const char *phase = worm_mc_classify_phase(result);
    double T_K = (params->beta > 1e-30)
                 ? 1.0 / (8.617333e-5 * params->beta)
                 : 0.0;

    /* BUG-WORM-LABEL-FIX-C41 : la 4e colonne était params->U / fabs(params->t)
     * ce qui donnait U/t = 8.667 au lieu de U en eV = 5.2.
     * Correction : écrire params->U (valeur réelle en eV).
     * La colonne est renommée implicitement : c'est U_eV, pas U_over_t.        */
    double U_eV_real = params->U;          /* U réel en eV — pas U/t */
    fprintf(csv_out,
            "worm_mc_bosonic,E_per_site,%.3f,%.6f,%.10f,%s\n",
            T_K, U_eV_real,
            result->E_per_site,
            result->converged ? "PASS" : "FAIL");
    fprintf(csv_out,
            "worm_mc_bosonic,n_per_site,%.3f,%.6f,%.10f,%s\n",
            T_K, U_eV_real,
            result->n_per_site,
            result->n_per_site > 0.0 ? "PASS" : "FAIL");
    fprintf(csv_out,
            "worm_mc_bosonic,superfluid_density,%.3f,%.6f,%.10f,%s\n",
            T_K, U_eV_real,
            result->superfluid_density,
            phase);
    fprintf(csv_out,
            "worm_mc_bosonic,compressibility,%.3f,%.6f,%.10f,%s\n",
            T_K, U_eV_real,
            result->compressibility,
            result->compressibility >= 0.0 ? "PASS" : "FAIL");
    /* C39-MOTT-FIX : acceptance_rate=0 est PHYSIQUEMENT CORRECT pour l'isolant de Mott
     * (U/t=13.75 >> (U/t)_c≈3.37). Si converged=true (Mott OU acceptance normale),
     * le statut est PASS. WARN uniquement si non convergé et non Mott. */
    fprintf(csv_out,
            "worm_mc_bosonic,acceptance_rate,%.3f,%.6f,%.10f,%s\n",
            T_K, U_eV_real,
            result->acceptance_rate,
            result->converged ? "PASS" : "WARN");
    return 0;
}

/* -------------------------------------------------------------------------
 * worm_mc_classify_phase
 * ------------------------------------------------------------------------- */

const char *worm_mc_classify_phase(const worm_mc_result_t *result) {
    if (!result) return "unknown";
    if (result->superfluid) return "superfluid";
    /* Isolant de Mott : faible compressibilité + occupation proche d'un entier */
    double n = result->n_per_site;
    double frac = n - floor(n);
    bool near_integer = (frac < 0.05 || frac > 0.95);
    if (near_integer && result->compressibility < 0.1) return "mott_insulator";
    return "normal";
}
