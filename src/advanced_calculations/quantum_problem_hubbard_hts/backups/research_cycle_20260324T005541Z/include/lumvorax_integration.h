/* BC-LV : LumVorax Integration Bridge — Hubbard HTS Simulator
 * Auto-contenu : aucune dépendance externe hors stdlib/pthread.
 * Activé par -DLUMVORAX_ENABLED=1 au moment de la compilation.
 * Sans ce flag, toutes les macros deviennent des no-ops : zéro overhead.
 *
 * Fournit :
 *   - Logging forensique nanoseconde par module/opération (CLOCK_MONOTONIC)
 *   - Tracking mémoire optionnel (malloc/calloc/free tracés)
 *   - Rapport de fuites en fin de session
 *   - Thread-safe via pthread_mutex_t
 */
#ifndef LUMVORAX_INTEGRATION_H
#define LUMVORAX_INTEGRATION_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================
 * API PUBLIQUE — disponible avec ou sans LUMVORAX_ENABLED
 * ====================================================== */
bool     lv_init(const char* log_dir);
void     lv_destroy(void);
void     lv_module_start(const char* file, int line, const char* func,
                         const char* module, const char* label);
void     lv_module_end(const char* file, int line, const char* func,
                       const char* module, const char* label, bool success);
void     lv_module_metric(const char* file, int line, const char* func,
                          const char* module, const char* metric, double value);
void     lv_module_operation(const char* file, int line, const char* func,
                             const char* module, const char* op, const char* data);
void*    lv_tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* func);
void*    lv_tracked_malloc(size_t size, const char* file, int line, const char* func);
void     lv_tracked_free(void* ptr, const char* file, int line, const char* func);
void     lv_report_leaks(void);
uint64_t lv_get_timestamp_ns(void);

/* ======================================================
 * MACROS FORENSIQUES — no-op si LUMVORAX_ENABLED absent
 * ====================================================== */
#ifdef LUMVORAX_ENABLED

#define LV_MODULE_START(module, label) \
    lv_module_start(__FILE__, __LINE__, __func__, (module), (label))

#define LV_MODULE_END(module, label, success) \
    lv_module_end(__FILE__, __LINE__, __func__, (module), (label), (success))

#define LV_MODULE_METRIC(module, metric, value) \
    lv_module_metric(__FILE__, __LINE__, __func__, (module), (metric), (double)(value))

#define LV_MODULE_OP(module, op, data) \
    lv_module_operation(__FILE__, __LINE__, __func__, (module), (op), (data))

#define LV_CALLOC(nmemb, size) \
    lv_tracked_calloc((nmemb), (size), __FILE__, __LINE__, __func__)

#define LV_MALLOC(size) \
    lv_tracked_malloc((size), __FILE__, __LINE__, __func__)

#define LV_FREE(ptr) \
    lv_tracked_free((ptr), __FILE__, __LINE__, __func__)

#else /* LUMVORAX_ENABLED not defined — stubs zero-overhead */

#define LV_MODULE_START(m, l)       ((void)0)
#define LV_MODULE_END(m, l, s)      ((void)0)
#define LV_MODULE_METRIC(m, k, v)   ((void)0)
#define LV_MODULE_OP(m, o, d)       ((void)0)
#define LV_CALLOC(n, s)             calloc((n), (s))
#define LV_MALLOC(s)                malloc(s)
#define LV_FREE(p)                  free(p)

#endif /* LUMVORAX_ENABLED */

#endif /* LUMVORAX_INTEGRATION_H */
