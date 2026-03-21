#ifndef ULTRA_FORENSIC_LOGGER_H
#define ULTRA_FORENSIC_LOGGER_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

// Niveaux de logging forensique ultra-stricts
typedef enum {
    FORENSIC_TRACE = 0,      // Traçage détaillé
    FORENSIC_DEBUG = 1,      // Debug développement
    FORENSIC_INFO = 2,       // Information générale
    FORENSIC_WARNING = 3,    // Avertissements
    FORENSIC_ERROR = 4,      // Erreurs récupérables
    FORENSIC_CRITICAL = 5,   // Erreurs critiques
    FORENSIC_EMERGENCY = 6   // Urgences système
} ultra_forensic_level_e;

// Structure pour tracking par module
typedef struct {
    char module_name[64];
    char test_name[128];
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    uint64_t operations_count;
    size_t memory_used;
    FILE* module_log_file;
    pthread_mutex_t mutex;
} module_forensic_tracker_t;

// Macros de logging obligatoires par module
#define FORENSIC_LOG_MODULE_START(module, test) \
    ultra_forensic_log_module_start(__FILE__, __LINE__, __func__, module, test)

#define FORENSIC_LOG_MODULE_END(module, test, result) \
    ultra_forensic_log_module_end(__FILE__, __LINE__, __func__, module, test, result)

#define FORENSIC_LOG_MODULE_OPERATION(module, operation, data) \
    ultra_forensic_log_module_operation(__FILE__, __LINE__, __func__, module, operation, data)

#define FORENSIC_LOG_MODULE_METRIC(module, metric_name, value) \
    ultra_forensic_log_module_metric(__FILE__, __LINE__, __func__, module, metric_name, value)

// Function declarations
void ultra_forensic_logger_init_lum(const char* log_file);
bool ultra_forensic_logger_init(void);
void ultra_forensic_logger_destroy(void);

void ultra_forensic_log_module_start(const char* file, int line, const char* func, 
                                   const char* module, const char* test);
void ultra_forensic_log_module_end(const char* file, int line, const char* func,
                                 const char* module, const char* test, bool success);
void ultra_forensic_log_module_operation(const char* file, int line, const char* func,
                                        const char* module, const char* operation, const char* data);
void ultra_forensic_log_module_metric(const char* file, int line, const char* func,
                                     const char* module, const char* metric_name, double value);

// Validation existence logs obligatoire
bool ultra_forensic_validate_all_logs_exist(void);
void ultra_forensic_generate_summary_report(void);

#endif // ULTRA_FORENSIC_LOGGER_H