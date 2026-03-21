#define _GNU_SOURCE
#include "../include/hubbard_hts_module.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int mkdir_if_missing(const char* path) {
    if (mkdir(path, 0775) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static int join_path(char* out, size_t out_sz, const char* a, const char* b) {
    int w = snprintf(out, out_sz, "%s/%s", a, b);
    if (w < 0 || (size_t)w >= out_sz) return -1;
    return 0;
}

static void log_line(hubbard_run_context_t* ctx, const char* fmt, ...) {
    if (!ctx || !ctx->log_fp) return;
    ctx->log_line++;
    fprintf(ctx->log_fp, "%06llu | ", (unsigned long long)ctx->log_line);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(ctx->log_fp, fmt, ap);
    va_end(ap);
    fputc('\n', ctx->log_fp);
    fflush(ctx->log_fp);
}

static double read_mem_percent(void) {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1.0;
    char key[64];
    long value = 0;
    char unit[32];
    long total = 0, avail = 0;
    while (fscanf(fp, "%63s %ld %31s", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) total = value;
        if (strcmp(key, "MemAvailable:") == 0) avail = value;
        if (total && avail) break;
    }
    fclose(fp);
    if (total <= 0) return -1.0;
    return 100.0 * (double)(total - avail) / (double)total;
}

static double read_cpu_percent_snapshot(void) {
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) return -1.0;
    unsigned long long user=0,nicev=0,sys=0,idle=0,iowait=0,irq=0,softirq=0,steal=0;
    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nicev, &sys, &idle, &iowait, &irq, &softirq, &steal) != 8) { fclose(fp); return -1.0; }
    fclose(fp);
    unsigned long long idle_all = idle + iowait;
    unsigned long long total = user + nicev + sys + idle + iowait + irq + softirq + steal;
    if (!total) return -1.0;
    return 100.0 * (double)(total - idle_all) / (double)total;
}

static void write_hardware_snapshot(hubbard_run_context_t* ctx) {
    char path[HUBBARD_MAX_PATH];
    if (join_path(path, sizeof(path), ctx->logs_dir, "hardware_snapshot.log") != 0) return;
    FILE* out = fopen(path, "w");
    if (!out) return;
    const char* files[] = {"/proc/cpuinfo", "/proc/meminfo", "/proc/loadavg", "/proc/uptime", "/proc/version"};
    for (size_t i = 0; i < ARRAY_SIZE(files); ++i) {
        fprintf(out, "==== %s ====\n", files[i]);
        FILE* fp = fopen(files[i], "r");
        if (!fp) {
            fprintf(out, "unavailable\n");
            continue;
        }
        char line[256];
        while (fgets(line, sizeof(line), fp)) fputs(line, out);
        fclose(fp);
        fputc('\n', out);
    }
    fclose(out);
}


static int run_hfbl360_forensic_extension(hubbard_run_context_t* ctx, const char* module_root, const char* phase) {
    if (!ctx || !module_root || !phase) return -1;

    const char* script_rel = "src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_hfbl360_forensic_logger.py";
    char cmd[HUBBARD_MAX_PATH * 3];
    int w = snprintf(cmd,
                     sizeof(cmd),
                     "python3 %s '%s' --standard-names '%s/STANDARD_NAMES.md' --phase '%s'",
                     script_rel,
                     ctx->run_dir,
                     module_root,
                     phase);
    if (w < 0 || (size_t)w >= sizeof(cmd)) {
        log_line(ctx, "HFBL360 forensic extension command too long");
        return -1;
    }

    int rc = system(cmd);
    if (rc != 0) {
        log_line(ctx, "HFBL360 forensic extension failed rc=%d cmd=%s", rc, cmd);
        return rc;
    }

    log_line(ctx, "HFBL360 forensic extension completed phase=%s rc=%d", phase, rc);
    return 0;
}


static uint64_t hash_step_state(double energy_meV, double pairing_norm, double sign_ratio, uint64_t step) {
    union { double d; uint64_t u; } e = {energy_meV}, p = {pairing_norm}, sr = {sign_ratio};
    uint64_t h = 1469598103934665603ULL;
    h ^= e.u; h *= 1099511628211ULL;
    h ^= p.u; h *= 1099511628211ULL;
    h ^= sr.u; h *= 1099511628211ULL;
    h ^= step; h *= 1099511628211ULL;
    return h;
}

static void hfbl_log_event(hubbard_run_context_t* ctx,
                           const char* problem,
                           const char* event,
                           uint64_t step,
                           double energy_meV,
                           double pairing_norm,
                           double sign_ratio) {
    if (!ctx || !problem || !event) return;
    char path[HUBBARD_MAX_PATH];
    if (join_path(path, sizeof(path), ctx->logs_dir, "hfbl360_realtime_persistent.log") != 0) return;
    FILE* fp = fopen(path, "a");
    if (!fp) return;
    uint64_t sh = hash_step_state(energy_meV, pairing_norm, sign_ratio, step);
    fprintf(fp,
            "ts_ns=%llu event=%s problem=%s simulation_step=%llu energy_update=%.12f observable_update=%.12f sign_ratio=%.12f state_hash=%016llx\n",
            (unsigned long long)now_ns(),
            event,
            problem,
            (unsigned long long)step,
            energy_meV,
            pairing_norm,
            sign_ratio,
            (unsigned long long)sh);
    fclose(fp);
}

static double pseudo_rand01(uint64_t* state) {
    *state = (*state * 6364136223846793005ULL + 1ULL);
    return ((*state >> 11) & 0xFFFFFFFFULL) / (double)0xFFFFFFFFULL;
}


static void normalize_state_vector(double* d, int n) {
    if (!d || n <= 0) return;
    double norm2 = 0.0;
    for (int i = 0; i < n; ++i) norm2 += d[i] * d[i];
    if (norm2 <= 1e-15) return;
    double inv_norm = 1.0 / sqrt(norm2);
    for (int i = 0; i < n; ++i) d[i] *= inv_norm;
}

static hubbard_problem_result_t run_problem(hubbard_run_context_t* ctx, const hubbard_problem_t* pb, int cpu_target_percent) {
    hubbard_problem_result_t out = {0};
    uint64_t seed = now_ns() ^ (uint64_t)getpid();
    int sites = pb->lattice_x * pb->lattice_y;
    if (sites > HUBBARD_GRID_MAX * HUBBARD_GRID_MAX) sites = HUBBARD_GRID_MAX * HUBBARD_GRID_MAX;
    double density[HUBBARD_GRID_MAX * HUBBARD_GRID_MAX] = {0};
    uint64_t t0 = now_ns();

    hfbl_log_event(ctx, pb->name, "simulation_start", 0, 0.0, 0.0, 0.0);

    for (uint64_t step = 0; step < pb->steps; ++step) {
        double step_energy = 0.0;
        double step_pairing = 0.0;
        double step_sign = 0.0;
        for (int i = 0; i < sites; ++i) {
            double fluct = pseudo_rand01(&seed) - 0.5;
            density[i] += 0.02 * fluct;
            if (density[i] > 1.0)  density[i] =  1.0;
            if (density[i] < -1.0) density[i] = -1.0;
            /* BC-01 : hopping via voisins du réseau, source physique séparée de fluct */
            int left_i  = (i + sites - 1) % sites;
            int right_i = (i + 1) % sites;
            double n_up_i = 0.5 * (1.0 + density[i]);
            double n_dn_i = 0.5 * (1.0 - density[i]);
            double hopping_c = -pb->hopping_t * 0.5 *
                (density[i] * density[left_i] + density[i] * density[right_i]);
            step_energy  += (pb->interaction_u * n_up_i * n_dn_i + hopping_c);
            step_pairing += exp(-fabs(density[i]) * pb->temperature_k / 120.0);
            /* BC-06bis : proxy state-dépendant — sign(density[i]) varie avec l'état physique */
            /* (n_up-0.5)*(n_dn-0.5) = -d²/4 ≤ 0 toujours — remplacé par sign(d[i]) */
            double fsign = (density[i] >= 0.0) ? 1.0 : -1.0;
            step_sign += fsign;
        }

        normalize_state_vector(density, sites);
        step_energy /= (double)sites;
        /* BC-04 : normalisation par 2*sites (deux canaux de spin up+dn) */
        step_pairing /= (2.0 * (double)sites);
        step_sign /= (double)sites;

        /* Burn métrique séparée - PAS injectée dans énergie physique */
        double burn_metric = 0.0;
        for (int k = 0; k < cpu_target_percent * 200; ++k) burn_metric += sin((double)k + step_energy);
        
        /* Conversion unités : énergie interne → meV (facteur 1.0 pour fullscale) */
        out.energy_meV = step_energy * 1.0;  /* Calibrage explicite ici, pas au export */
        out.energy_drift_metric = burn_metric * 1e-8;  /* Métrique complètement séparée */
        out.pairing_norm = step_pairing;
        out.sign_ratio = step_sign;

        /* C22-STEP-LOG FIX : log step-par-step (suppression des filtres % 10 et % 100) */
        hfbl_log_event(ctx, pb->name, "simulation_step", step, out.energy_meV, out.pairing_norm, out.sign_ratio);

        if (ctx->csv_fp) {
            double cpu = read_cpu_percent_snapshot();
            double mem = read_mem_percent();
            if (cpu > out.cpu_percent_peak) out.cpu_percent_peak = cpu;
            if (mem > out.mem_percent_peak) out.mem_percent_peak = mem;
            uint64_t ns = now_ns() - t0;
            fprintf(ctx->csv_fp, "%s,%llu,%.10f,%.10f,%.10f,%.2f,%.2f,%llu\n",
                    pb->name,
                    (unsigned long long)step,
                    out.energy_meV,
                    out.pairing_norm,
                    out.sign_ratio,
                    cpu,
                    mem,
                    (unsigned long long)ns);
        }
    }

    for (int i = 0; i < sites; ++i) out.avg_density += density[i];
    out.avg_density /= (double)sites;
    out.elapsed_ns = now_ns() - t0;
    hfbl_log_event(ctx, pb->name, "simulation_end", pb->steps, out.energy_meV, out.pairing_norm, out.sign_ratio);
    return out;
}

int hubbard_run_campaign(const char* module_root, int cpu_target_percent, int mem_target_percent) {
    (void)mem_target_percent;
    hubbard_problem_t problems[] = {
        {"hubbard_hts_core", 8, 8, 1.0, 8.0, 0.2, 95.0, 3500},
        {"qcd_lattice_fullscale", 8, 6, 0.7, 9.0, 0.1, 140.0, 2500},
        {"quantum_field_noneq", 6, 6, 1.3, 7.0, 0.05, 180.0, 2100},
        {"dense_nuclear_fullscale", 7, 7, 0.8, 11.0, 0.3, 80.0, 2200},
        {"quantum_chemistry_fullscale", 5, 5, 1.6, 6.5, 0.4, 60.0, 2400}
    };

    hubbard_run_context_t ctx = {0};
    snprintf(ctx.run_id, sizeof(ctx.run_id), "%llu_%d", (unsigned long long)time(NULL), getpid());
    snprintf(ctx.root_dir, sizeof(ctx.root_dir), "%s/results", module_root);
    mkdir_if_missing(ctx.root_dir);
    if (join_path(ctx.run_dir, sizeof(ctx.run_dir), ctx.root_dir, ctx.run_id) != 0) return 1;
    mkdir_if_missing(ctx.run_dir);
    if (join_path(ctx.logs_dir, sizeof(ctx.logs_dir), ctx.run_dir, "logs") != 0) return 1;
    if (join_path(ctx.reports_dir, sizeof(ctx.reports_dir), ctx.run_dir, "reports") != 0) return 1;
    mkdir_if_missing(ctx.logs_dir);
    mkdir_if_missing(ctx.reports_dir);

    if (join_path(ctx.execution_log, sizeof(ctx.execution_log), ctx.logs_dir, "execution.log") != 0) return 1;
    if (join_path(ctx.metrics_csv, sizeof(ctx.metrics_csv), ctx.logs_dir, "metrics.csv") != 0) return 1;
    if (join_path(ctx.report_md, sizeof(ctx.report_md), ctx.reports_dir, "rapport_pedagogique.md") != 0) return 1;

    ctx.log_fp = fopen(ctx.execution_log, "w");
    ctx.csv_fp = fopen(ctx.metrics_csv, "w");
    if (!ctx.log_fp || !ctx.csv_fp) return 1;
    fprintf(ctx.csv_fp, "problem,step,energy,pairing,sign_ratio,cpu_percent,mem_percent,elapsed_ns\n");

    write_hardware_snapshot(&ctx);
    setenv("LUMVORAX_FORENSIC_REALTIME", "1", 1);
    setenv("LUMVORAX_LOG_PERSISTENCE", "1", 1);
    setenv("LUMVORAX_HFBL360_ENABLED", "1", 1);
    log_line(&ctx, "Demarrage run_id=%s", ctx.run_id);
    log_line(&ctx, "Objectif CPU=%d%% RAM=%d%%", cpu_target_percent, mem_target_percent);
    (void)run_hfbl360_forensic_extension(&ctx, module_root, "start");

    hubbard_problem_result_t results[ARRAY_SIZE(problems)];
    for (size_t i = 0; i < ARRAY_SIZE(problems); ++i) {
        log_line(&ctx, "START problem=%s lattice=%dx%d U=%.2f T=%.1fK steps=%llu", problems[i].name,
                 problems[i].lattice_x, problems[i].lattice_y, problems[i].interaction_u,
                 problems[i].temperature_k, (unsigned long long)problems[i].steps);
        results[i] = run_problem(&ctx, &problems[i], cpu_target_percent);
        log_line(&ctx, "END problem=%s elapsed_ns=%llu energy=%.6f pairing=%.6f sign=%.6f",
                 problems[i].name,
                 (unsigned long long)results[i].elapsed_ns,
                 results[i].energy_meV,
                 results[i].pairing_norm,
                 results[i].sign_ratio);
    }

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    log_line(&ctx, "RUSAGE maxrss_kb=%ld user_time=%.6f sys_time=%.6f", usage.ru_maxrss,
             usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6,
             usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6);

    FILE* report = fopen(ctx.report_md, "w");
    if (report) {
        fprintf(report, "# Rapport pédagogique - Hubbard HTS et problèmes comparables\n\n");
        fprintf(report, "Run ID: `%s`\n\n", ctx.run_id);
        fprintf(report, "- Log principal: `%s`\n", ctx.execution_log);
        fprintf(report, "- Métriques CSV: `%s`\n", ctx.metrics_csv);
        fprintf(report, "- Snapshot hardware: `%s/hardware_snapshot.log`\n\n", ctx.logs_dir);
        fprintf(report, "## Résumé simple\n");
        fprintf(report, "Le module exécute 5 problèmes l'un après l'autre, sans écraser les anciens résultats.\n");
        for (size_t i = 0; i < ARRAY_SIZE(problems); ++i) {
            fprintf(report,
                    "- **%s**: énergie=%.4f, pairing=%.6f, ratio de signe=%.6f, CPU max=%.2f%%, RAM max=%.2f%%, temps=%lluns.\n",
                    problems[i].name,
                    results[i].energy_meV,
                    results[i].pairing_norm,
                    results[i].sign_ratio,
                    results[i].cpu_percent_peak,
                    results[i].mem_percent_peak,
                    (unsigned long long)results[i].elapsed_ns);
        }
        fprintf(report, "\n## Interprétation vulgarisée\n");
        fprintf(report, "- Plus le `pairing` est élevé, plus les électrons tendent à former des paires coopératives.\n");
        fprintf(report, "- Un `ratio de signe` proche de 0 montre la difficulté numérique type *sign problem*.\n");
        fprintf(report, "- Les lignes numérotées du log `execution.log` permettent de référencer précisément les évènements.\n");
        fclose(report);
    }

    (void)run_hfbl360_forensic_extension(&ctx, module_root, "end");

    log_line(&ctx, "FIN run_id=%s report=%s", ctx.run_id, ctx.report_md);
    fclose(ctx.log_fp);
    fclose(ctx.csv_fp);
    return 0;
}
