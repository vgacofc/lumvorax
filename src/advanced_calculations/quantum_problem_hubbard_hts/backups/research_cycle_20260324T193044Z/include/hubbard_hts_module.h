#ifndef HUBBARD_HTS_MODULE_H
#define HUBBARD_HTS_MODULE_H

#include <stdint.h>
#include <stdio.h>

#define HUBBARD_MAX_NAME 96
#define HUBBARD_MAX_PATH 512
#define HUBBARD_GRID_MAX 10

typedef struct {
    char name[HUBBARD_MAX_NAME];
    int lattice_x;
    int lattice_y;
    double hopping_t;
    double interaction_u;
    double chemical_mu;
    double temperature_k;
    uint64_t steps;
} hubbard_problem_t;

typedef struct {
    char run_id[64];
    char root_dir[HUBBARD_MAX_PATH];
    char run_dir[HUBBARD_MAX_PATH];
    char logs_dir[HUBBARD_MAX_PATH];
    char reports_dir[HUBBARD_MAX_PATH];
    char metrics_csv[HUBBARD_MAX_PATH];
    char execution_log[HUBBARD_MAX_PATH];
    char report_md[HUBBARD_MAX_PATH];
    FILE* log_fp;
    FILE* csv_fp;
    uint64_t log_line;
} hubbard_run_context_t;

typedef struct {
    double energy_meV;         /* Énergie physique en meV (unité explicite) */
    double energy_drift_metric; /* Métrique de dérive séparée du burn */
    double pairing_norm;       /* Observable appairage normalisé */
    double avg_density;
    double sign_ratio;
    double cpu_percent_peak;
    double mem_percent_peak;
    uint64_t elapsed_ns;
} hubbard_problem_result_t;

int hubbard_run_campaign(const char* module_root, int cpu_target_percent, int mem_target_percent);

#endif
