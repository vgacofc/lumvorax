#ifndef LUM_LOGGER_H
#define LUM_LOGGER_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "../lum/lum_core.h"
#include "../vorax/vorax_operations.h"
#include <stdint.h>
#include <stddef.h>

// Système de désactivation des logs pour benchmark performance
#ifdef DISABLE_LOGGING
#define lum_log(level, format, ...) do { } while(0)
#define lum_logf(level, format, ...) do { } while(0)
#define lum_log_init(filename) (true)
#define lum_log_destroy() do { } while(0)
#define lum_log_flush() do { } while(0)
#else
// Log levels
typedef enum {
    LUM_LOG_DEBUG = 0,
    LUM_LOG_INFO = 1,
    LUM_LOG_WARN = 2,
    LUM_LOG_ERROR = 3
} lum_log_level_e;

// Log entry types
typedef enum {
    LUM_LOG_OPERATION = 0,    // VORAX operation executed
    LUM_LOG_LUM_CREATE = 1,   // LUM created
    LUM_LOG_LUM_DESTROY = 2,  // LUM destroyed
    LUM_LOG_GROUP_CREATE = 3, // Group created
    LUM_LOG_GROUP_DESTROY = 4,// Group destroyed
    LUM_LOG_ZONE_CREATE = 5,  // Zone created
    LUM_LOG_ZONE_DESTROY = 6, // Zone destroyed
    LUM_LOG_MEMORY_STORE = 7, // Memory store operation
    LUM_LOG_MEMORY_RETRIEVE = 8, // Memory retrieve operation
    LUM_LOG_CONSERVATION = 9, // Conservation check
    LUM_LOG_ERROR_EVENT = 10  // Error events
} lum_log_entry_type_e;

// Log entry structure
typedef struct {
    uint64_t timestamp;
    uint32_t sequence_id;
    lum_log_level_e level;
    lum_log_entry_type_e type;
    char operation[32];       // Operation name (fuse, split, etc.)
    uint32_t lum_ids[16];     // Affected LUM IDs (max 16)
    size_t lum_count;         // Number of affected LUMs
    char zone_names[4][32];   // Affected zone names (max 4)
    size_t zone_count;        // Number of affected zones
    char memory_name[32];     // Affected memory name
    char message[256];        // Log message
    bool conservation_valid;  // Conservation check result
    size_t input_lum_count;   // Input LUM count for operations
    size_t output_lum_count;  // Output LUM count for operations
} lum_log_entry_t;

// Logger context
typedef struct {
    FILE* log_file;
    char log_filename[256];
    bool console_output;
    bool file_output;
    lum_log_level_e min_level;
    uint32_t sequence_counter;
    bool trace_all_lums;
    bool conservation_check;
    lum_log_level_e level;              // Niveau de log
    bool enabled;                       // État activé/désactivé
    char module_name[64];               // Nom du module
    char session_id[32];                // ID de session
} lum_logger_t;

// Logger initialization and cleanup
lum_logger_t* lum_logger_create(const char* log_filename, bool console_output, bool file_output);
void lum_logger_destroy(lum_logger_t* logger);
bool lum_logger_set_level(lum_logger_t* logger, lum_log_level_e level);
bool lum_logger_enable_tracing(lum_logger_t* logger, bool enable);

// Core logging functions
void lum_log_operation(lum_logger_t* logger, vorax_operation_e operation, 
                       const lum_group_t** input_groups, size_t input_count,
                       const lum_group_t** output_groups, size_t output_count,
                       const char* zones[], size_t zone_count);

void lum_log_lum_event(lum_logger_t* logger, lum_log_entry_type_e type,
                       const lum_t* lum, const char* message);

void lum_log_group_event(lum_logger_t* logger, lum_log_entry_type_e type,
                         const lum_group_t* group, const char* message);

void lum_log_zone_event(lum_logger_t* logger, lum_log_entry_type_e type,
                        const char* zone_name, const char* message);

void lum_log_memory_event(lum_logger_t* logger, lum_log_entry_type_e type,
                          const char* memory_name, const lum_group_t* group,
                          const char* message);

void lum_log_conservation_check(lum_logger_t* logger, 
                                const lum_group_t** input_groups, size_t input_count,
                                const lum_group_t** output_groups, size_t output_count,
                                bool conservation_valid);

void lum_log_error(lum_logger_t* logger, const char* error_message);

// Convenience logging macros
#define LUM_LOG_DEBUG_MSG(logger, msg) \
    if (logger && logger->min_level <= LUM_LOG_DEBUG) \
        lum_log_message(logger, LUM_LOG_DEBUG, msg)

#define LUM_LOG_INFO_MSG(logger, msg) \
    if (logger && logger->min_level <= LUM_LOG_INFO) \
        lum_log_message(logger, LUM_LOG_INFO, msg)

#define LUM_LOG_WARN_MSG(logger, msg) \
    if (logger && logger->min_level <= LUM_LOG_WARN) \
        lum_log_message(logger, LUM_LOG_WARN, msg)

#define LUM_LOG_ERROR_MSG(logger, msg) \
    if (logger && logger->min_level <= LUM_LOG_ERROR) \
        lum_log_message(logger, LUM_LOG_ERROR, msg)

// Log analysis functions
typedef struct {
    size_t total_operations;
    size_t total_lums_created;
    size_t total_lums_destroyed;
    size_t conservation_violations;
    size_t error_count;
    uint64_t start_timestamp;
    uint64_t end_timestamp;
    char most_used_operation[32];
    size_t operation_counts[16];
} lum_log_analysis_t;

lum_log_analysis_t* lum_log_analyze(const char* log_filename);
void lum_log_print_analysis(const lum_log_analysis_t* analysis);
void lum_log_analysis_destroy(lum_log_analysis_t* analysis);

// Log replay functions
typedef struct {
    lum_log_entry_t* entries;
    size_t entry_count;
    size_t entry_capacity;
} lum_log_replay_t;

lum_log_replay_t* lum_log_replay_create(const char* log_filename);
void lum_log_replay_destroy(lum_log_replay_t* replay);
// Forward declaration
struct vorax_execution_context;
bool lum_log_replay_execute(lum_log_replay_t* replay, struct vorax_execution_context* ctx);

// Global logger functions
void lum_set_global_logger(lum_logger_t* logger);
lum_logger_t* lum_get_global_logger(void);

// Utility functions
void lum_log_message(lum_logger_t* logger, lum_log_level_e level, const char* message);
const char* lum_log_level_to_string(lum_log_level_e level);
const char* lum_log_entry_type_to_string(lum_log_entry_type_e type);
const char* vorax_operation_to_string(vorax_operation_e operation);

// CSV export functions
bool lum_log_export_csv(const char* log_filename, const char* csv_filename);
bool lum_log_export_json(const char* log_filename, const char* json_filename);

// Live monitoring
typedef struct {
    lum_logger_t* logger;
    bool monitoring;
    void (*callback)(const lum_log_entry_t* entry, void* user_data);
    void* user_data;
} lum_log_monitor_t;

lum_log_monitor_t* lum_log_monitor_create(lum_logger_t* logger,
                                          void (*callback)(const lum_log_entry_t*, void*),
                                          void* user_data);
void lum_log_monitor_destroy(lum_log_monitor_t* monitor);
bool lum_log_monitor_start(lum_log_monitor_t* monitor);
void lum_log_monitor_stop(lum_log_monitor_t* monitor);

void lum_log_flush(void);

// Fonction lum_log principale pour compatibilité
void lum_log(lum_log_level_e level, const char* format, ...);

// Variadic logging helper for replacing incorrect snprintf usage
void lum_logf(lum_log_level_e level, const char* format, ...);

// Convenience macros
#define LOG_INFOF(fmt, ...) lum_logf(LUM_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) lum_logf(LUM_LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...) lum_logf(LUM_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_DEBUGF(fmt, ...) lum_logf(LUM_LOG_DEBUG, fmt, ##__VA_ARGS__)

#endif // DISABLE_LOGGING

#endif // LUM_LOGGER_H