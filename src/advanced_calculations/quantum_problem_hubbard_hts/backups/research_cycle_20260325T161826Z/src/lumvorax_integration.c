/* BC-LV : LumVorax Integration Bridge — Implémentation auto-contenus
 * Dépendances : stdlib/pthread/time uniquement. Aucune dépendance externe.
 * Thread-safe via pthread_mutex_t. Timestamps CLOCK_MONOTONIC (résolution 1 ns).
 *
 * Corrections appliquées :
 *   BC-LV01 : Ce fichier est la liaison compilée qui remplace les 5 sources
 *              LumVorax dispersées (forensic_logger, ultra_forensic_logger,
 *              memory_tracker, lum_core, lum_logger) par un bridge unique
 *              sans dépendances circulaires.
 *   BC-LV02 : Les fonctions lire getenv() dans hubbard_hts_research_cycle.c
 *   BC-LV03 : lv_init() / lv_destroy() appelés dans main()
 *   BC-LV04 : LV_MODULE_START/END/METRIC/OP aux 6 points critiques
 *   BC-LV05 : LV_CALLOC/LV_FREE remplacent calloc/free dans les boucles
 */

#define _POSIX_C_SOURCE 200809L

#include "../include/lumvorax_integration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

/* ======================================================
 * CONSTANTES INTERNES
 * ====================================================== */
#define LV_MAX_MODULES       64
#define LV_MAX_ALLOC_ENTRIES 20000
#define LV_LOG_BUF           512

/* ======================================================
 * STRUCTURES INTERNES
 * ====================================================== */
typedef struct {
    char   module_name[64];
    char   label[128];
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t operations;
    size_t   memory_used;
    bool     active;
} lv_module_tracker_t;

typedef struct {
    void*       ptr;
    size_t      size;
    const char* file;
    int         line;
    const char* func;
    bool        freed;
} lv_alloc_entry_t;

/* ======================================================
 * ÉTAT GLOBAL
 * ====================================================== */
static pthread_mutex_t   g_mutex          = PTHREAD_MUTEX_INITIALIZER;
static bool              g_initialized    = false;
static FILE*             g_log_file       = NULL;
static lv_module_tracker_t g_modules[LV_MAX_MODULES];
static int               g_module_count   = 0;
static lv_alloc_entry_t  g_allocs[LV_MAX_ALLOC_ENTRIES];
static int               g_alloc_count    = 0;
static size_t            g_total_alloc    = 0;
static size_t            g_total_freed    = 0;
static bool              g_memtrack_on    = false;

/* ======================================================
 * TIMESTAMP NANOSECONDE — CLOCK_MONOTONIC
 * ====================================================== */
uint64_t lv_get_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

/* ======================================================
 * INITIALISATION / DESTRUCTION
 * ====================================================== */
static void lv_mkdir_p(const char* path) {
    char tmp[512];
    char* p;
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

bool lv_init(const char* log_dir) {
    pthread_mutex_lock(&g_mutex);
    if (g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return true;
    }

    const char* dir = (log_dir && log_dir[0]) ? log_dir : "logs/forensic";
    lv_mkdir_p(dir);

    char log_path[512];
    uint64_t ts = lv_get_timestamp_ns();
    snprintf(log_path, sizeof(log_path), "%s/lumvorax_hubbard_hts_%" PRIu64 ".log", dir, ts);

    g_log_file = fopen(log_path, "w");
    if (!g_log_file) {
        /* Fallback vers stderr uniquement */
        fprintf(stderr, "[LUMVORAX] WARNING: cannot create log %s — using stderr only\n", log_path);
    }

    memset(g_modules, 0, sizeof(g_modules));
    memset(g_allocs,  0, sizeof(g_allocs));
    g_module_count = 0;
    g_alloc_count  = 0;
    g_total_alloc  = 0;
    g_total_freed  = 0;

    /* Activer le tracking mémoire si variable d'env présente */
    const char* env_mem = getenv("LUMVORAX_MEMORY_TRACKER");
    g_memtrack_on = (env_mem && env_mem[0] == '1');

    g_initialized = true;

    char iso[32];
    time_t tnow = time(NULL);
    struct tm g;
    gmtime_r(&tnow, &g);
    snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             g.tm_year + 1900, g.tm_mon + 1, g.tm_mday,
             g.tm_hour, g.tm_min, g.tm_sec);

    if (g_log_file) {
        fprintf(g_log_file,
                "=== LUMVORAX FORENSIC LOG STARTED ===\n"
                "timestamp_utc=%s\ntimestamp_ns=%" PRIu64 "\n"
                "memory_tracking=%s\nlog_path=%s\n"
                "=======================================\n",
                iso, ts, g_memtrack_on ? "ENABLED" : "DISABLED", log_path);
        fflush(g_log_file);
    }
    fprintf(stderr, "[LUMVORAX] Forensic logger initialized — %s — memtrack=%s\n",
            iso, g_memtrack_on ? "ON" : "OFF");

    pthread_mutex_unlock(&g_mutex);
    return true;
}

void lv_destroy(void) {
    pthread_mutex_lock(&g_mutex);
    if (!g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    if (g_log_file) {
        fprintf(g_log_file, "=== LUMVORAX FORENSIC LOG ENDED ===\n");
        fprintf(g_log_file, "total_alloc_bytes=%zu total_freed_bytes=%zu\n",
                g_total_alloc, g_total_freed);
        fclose(g_log_file);
        g_log_file = NULL;
    }
    g_initialized = false;
    fprintf(stderr, "[LUMVORAX] Forensic logger destroyed. alloc=%zu freed=%zu\n",
            g_total_alloc, g_total_freed);
    pthread_mutex_unlock(&g_mutex);
}

/* ======================================================
 * LOGGING INTERNE
 * ====================================================== */
static void lv_write(const char* file, int line, const char* func,
                     const char* level, const char* module, const char* msg) {
    uint64_t ts = lv_get_timestamp_ns();
    pthread_mutex_lock(&g_mutex);
    if (g_log_file) {
        fprintf(g_log_file, "[%" PRIu64 " ns][%s][%s] %s:%d %s() — %s\n",
                ts, level, module, file, line, func, msg);
        fflush(g_log_file);
    }
    pthread_mutex_unlock(&g_mutex);
}

/* ======================================================
 * MODULE TRACKING
 * ====================================================== */
void lv_module_start(const char* file, int line, const char* func,
                     const char* module, const char* label) {
    if (!g_initialized) return;
    pthread_mutex_lock(&g_mutex);

    lv_module_tracker_t* t = NULL;
    for (int i = 0; i < g_module_count; ++i) {
        if (strcmp(g_modules[i].module_name, module) == 0) {
            t = &g_modules[i];
            break;
        }
    }
    if (!t && g_module_count < LV_MAX_MODULES) {
        t = &g_modules[g_module_count++];
        strncpy(t->module_name, module, sizeof(t->module_name) - 1);
    }

    if (t) {
        strncpy(t->label, label, sizeof(t->label) - 1);
        t->start_ns    = lv_get_timestamp_ns();
        t->operations  = 0;
        t->memory_used = 0;
        t->active      = true;
    }

    uint64_t ts = lv_get_timestamp_ns();
    if (g_log_file) {
        fprintf(g_log_file, "[%" PRIu64 " ns][MODULE_START][%s] %s:%d %s() label=%s\n",
                ts, module, file, line, func, label);
        fflush(g_log_file);
    }
    pthread_mutex_unlock(&g_mutex);
}

void lv_module_end(const char* file, int line, const char* func,
                   const char* module, const char* label, bool success) {
    if (!g_initialized) return;
    pthread_mutex_lock(&g_mutex);

    uint64_t end_ns = lv_get_timestamp_ns();
    uint64_t start_ns = end_ns;
    uint64_t ops = 0;

    for (int i = 0; i < g_module_count; ++i) {
        if (strcmp(g_modules[i].module_name, module) == 0) {
            g_modules[i].end_ns = end_ns;
            g_modules[i].active = false;
            start_ns = g_modules[i].start_ns;
            ops = g_modules[i].operations;
            break;
        }
    }

    if (g_log_file) {
        fprintf(g_log_file,
                "[%" PRIu64 " ns][MODULE_END][%s] %s:%d %s() label=%s "
                "duration_ns=%" PRIu64 " ops=%" PRIu64 " success=%s\n",
                end_ns, module, file, line, func, label,
                end_ns - start_ns, ops, success ? "true" : "false");
        fflush(g_log_file);
    }
    pthread_mutex_unlock(&g_mutex);
    (void)label;
}

void lv_module_metric(const char* file, int line, const char* func,
                      const char* module, const char* metric, double value) {
    if (!g_initialized) return;
    pthread_mutex_lock(&g_mutex);

    for (int i = 0; i < g_module_count; ++i) {
        if (strcmp(g_modules[i].module_name, module) == 0) {
            g_modules[i].operations++;
            break;
        }
    }

    if (g_log_file) {
        fprintf(g_log_file,
                "[%" PRIu64 " ns][METRIC][%s] %s:%d %s() %s=%.10g\n",
                lv_get_timestamp_ns(), module, file, line, func, metric, value);
        fflush(g_log_file);
    }
    pthread_mutex_unlock(&g_mutex);
}

void lv_module_operation(const char* file, int line, const char* func,
                         const char* module, const char* op, const char* data) {
    if (!g_initialized) return;
    char buf[LV_LOG_BUF];
    snprintf(buf, sizeof(buf), "op=%s data=%s", op, data ? data : "");
    lv_write(file, line, func, "OPERATION", module, buf);
}

/* ======================================================
 * MEMORY TRACKING
 * ====================================================== */
void* lv_tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* func) {
    void* ptr = calloc(nmemb, size);
    if (!ptr || !g_initialized || !g_memtrack_on) return ptr;

    pthread_mutex_lock(&g_mutex);
    if (g_alloc_count < LV_MAX_ALLOC_ENTRIES) {
        lv_alloc_entry_t* e = &g_allocs[g_alloc_count++];
        e->ptr   = ptr;
        e->size  = nmemb * size;
        e->file  = file;
        e->line  = line;
        e->func  = func;
        e->freed = false;
        g_total_alloc += e->size;

        if (g_log_file) {
            fprintf(g_log_file,
                    "[%" PRIu64 " ns][CALLOC][%s] %s:%d %s() ptr=%p size=%zu\n",
                    lv_get_timestamp_ns(), "memory", file, line, func, ptr, e->size);
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return ptr;
}

void* lv_tracked_malloc(size_t size, const char* file, int line, const char* func) {
    void* ptr = malloc(size);
    if (!ptr || !g_initialized || !g_memtrack_on) return ptr;

    pthread_mutex_lock(&g_mutex);
    if (g_alloc_count < LV_MAX_ALLOC_ENTRIES) {
        lv_alloc_entry_t* e = &g_allocs[g_alloc_count++];
        e->ptr   = ptr;
        e->size  = size;
        e->file  = file;
        e->line  = line;
        e->func  = func;
        e->freed = false;
        g_total_alloc += size;

        if (g_log_file) {
            fprintf(g_log_file,
                    "[%" PRIu64 " ns][MALLOC][%s] %s:%d %s() ptr=%p size=%zu\n",
                    lv_get_timestamp_ns(), "memory", file, line, func, ptr, size);
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return ptr;
}

void lv_tracked_free(void* ptr, const char* file, int line, const char* func) {
    if (!ptr) return;

    if (g_initialized && g_memtrack_on) {
        pthread_mutex_lock(&g_mutex);
        bool found = false;
        for (int i = 0; i < g_alloc_count; ++i) {
            if (g_allocs[i].ptr == ptr && !g_allocs[i].freed) {
                g_allocs[i].freed = true;
                g_total_freed += g_allocs[i].size;
                found = true;
                if (g_log_file) {
                    fprintf(g_log_file,
                            "[%" PRIu64 " ns][FREE][%s] %s:%d %s() ptr=%p size=%zu\n",
                            lv_get_timestamp_ns(), "memory", file, line, func,
                            ptr, g_allocs[i].size);
                }
                break;
            }
        }
        if (!found && g_log_file) {
            fprintf(g_log_file,
                    "[%" PRIu64 " ns][FREE_UNTRACKED][%s] %s:%d %s() ptr=%p\n",
                    lv_get_timestamp_ns(), "memory", file, line, func, ptr);
        }
        pthread_mutex_unlock(&g_mutex);
    }

    free(ptr);
}

/* ======================================================
 * RAPPORT DE FUITES
 * ====================================================== */
void lv_report_leaks(void) {
    if (!g_initialized) return;
    pthread_mutex_lock(&g_mutex);

    int leaks = 0;
    size_t leaked_bytes = 0;
    for (int i = 0; i < g_alloc_count; ++i) {
        if (!g_allocs[i].freed) {
            leaks++;
            leaked_bytes += g_allocs[i].size;
            if (g_log_file) {
                fprintf(g_log_file,
                        "[LEAK] ptr=%p size=%zu allocated at %s:%d in %s()\n",
                        g_allocs[i].ptr, g_allocs[i].size,
                        g_allocs[i].file, g_allocs[i].line, g_allocs[i].func);
            }
        }
    }

    if (g_log_file) {
        fprintf(g_log_file,
                "[LEAK_SUMMARY] leaks=%d leaked_bytes=%zu total_alloc=%zu total_freed=%zu\n",
                leaks, leaked_bytes, g_total_alloc, g_total_freed);
        fflush(g_log_file);
    }
    fprintf(stderr, "[LUMVORAX] Leak report: leaks=%d leaked_bytes=%zu\n",
            leaks, leaked_bytes);
    pthread_mutex_unlock(&g_mutex);
}
