
#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#define MAX_MODULE_LOGGERS 32

typedef struct {
    char module_name[64];
    char log_filename[256];
    FILE* log_file;
    bool enabled;
    size_t message_count;
} module_logger_t;

typedef struct {
    char session_id[32];
    module_logger_t module_loggers[MAX_MODULE_LOGGERS];
    size_t module_count;
    bool auto_archive;
    bool initialized;
} log_manager_t;

// Fonctions principales
log_manager_t* log_manager_create(void);
bool log_manager_create_structure(log_manager_t* manager);
bool log_manager_archive_session(log_manager_t* manager, const char* session_id);
module_logger_t* log_manager_get_module_logger(log_manager_t* manager, const char* module_name);
void log_manager_write_module_log(log_manager_t* manager, const char* module_name, 
                                  const char* level, const char* message);
void log_manager_destroy(log_manager_t* manager);
log_manager_t* log_manager_get_instance(void);

// Fonction utilitaire
void LOG_MODULE(const char* module, const char* level, const char* format, ...);

// Macros pratiques
#define LOG_LUM_CORE(level, ...) LOG_MODULE("lum_core", level, __VA_ARGS__)
#define LOG_VORAX(level, ...) LOG_MODULE("vorax_operations", level, __VA_ARGS__)
#define LOG_PARSER(level, ...) LOG_MODULE("parser", level, __VA_ARGS__)
#define LOG_BINARY(level, ...) LOG_MODULE("binary_converter", level, __VA_ARGS__)
#define LOG_OPTIMIZATION(level, ...) LOG_MODULE("optimization", level, __VA_ARGS__)
#define LOG_CRYPTO(level, ...) LOG_MODULE("crypto", level, __VA_ARGS__)
#define LOG_DEBUG(level, ...) LOG_MODULE("debug", level, __VA_ARGS__)

#endif // LOG_MANAGER_H
