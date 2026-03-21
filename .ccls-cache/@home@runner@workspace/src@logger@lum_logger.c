#include "lum_logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t global_sequence_counter = 1;

// Global logger instance for system-wide logging
static lum_logger_t* g_system_logger = NULL;

// Forward declaration of internal function
static void lum_log_write_entry(lum_logger_t* logger, const lum_log_entry_t* entry);

// Global logger functions
void lum_set_global_logger(lum_logger_t* logger) {
    g_system_logger = logger;
}

lum_logger_t* lum_get_global_logger(void) {
    return g_system_logger;
}

// Logger creation and management
lum_logger_t* lum_logger_create(const char* log_filename, bool console_output, bool file_output) {
    lum_logger_t* logger = malloc(sizeof(lum_logger_t));
    if (!logger) return NULL;
    
    logger->log_file = NULL;
    logger->console_output = console_output;
    logger->file_output = file_output;
    logger->min_level = LUM_LOG_INFO;
    logger->sequence_counter = 0;
    logger->trace_all_lums = false;
    logger->conservation_check = true;
    
    if (log_filename) {
        strncpy(logger->log_filename, log_filename, sizeof(logger->log_filename) - 1);
        logger->log_filename[sizeof(logger->log_filename) - 1] = '\0';
        
        if (file_output) {
            logger->log_file = fopen(log_filename, "a");
            if (!logger->log_file) {
                free(logger);
                return NULL;
            }
        }
    } else {
        logger->log_filename[0] = '\0';
    }
    
    return logger;
}

void lum_logger_destroy(lum_logger_t* logger) {
    if (logger) {
        if (logger->log_file) {
            fclose(logger->log_file);
        }
        free(logger);
    }
}

bool lum_logger_set_level(lum_logger_t* logger, lum_log_level_e level) {
    if (!logger) return false;
    logger->min_level = level;
    return true;
}

bool lum_logger_enable_tracing(lum_logger_t* logger, bool enable) {
    if (!logger) return false;
    logger->trace_all_lums = enable;
    return true;
}

// Core logging functions
void lum_log_operation(lum_logger_t* logger, vorax_operation_e operation, 
                       const lum_group_t** input_groups, size_t input_count,
                       const lum_group_t** output_groups, size_t output_count,
                       const char* zones[], size_t zone_count) {
    if (!logger) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = LUM_LOG_INFO;
    entry.type = LUM_LOG_OPERATION;
    
    strncpy(entry.operation, vorax_operation_to_string(operation), sizeof(entry.operation) - 1);
    
    // Count total input and output LUMs
    entry.input_lum_count = 0;
    entry.output_lum_count = 0;
    
    for (size_t i = 0; i < input_count; i++) {
        if (input_groups[i]) {
            entry.input_lum_count += input_groups[i]->count;
        }
    }
    
    for (size_t i = 0; i < output_count; i++) {
        if (output_groups[i]) {
            entry.output_lum_count += output_groups[i]->count;
        }
    }
    
    // Record affected LUM IDs (up to 16)
    entry.lum_count = 0;
    for (size_t g = 0; g < input_count && entry.lum_count < 16; g++) {
        if (input_groups[g]) {
            for (size_t i = 0; i < input_groups[g]->count && entry.lum_count < 16; i++) {
                entry.lum_ids[entry.lum_count++] = input_groups[g]->lums[i].id;
            }
        }
    }
    
    // Record zone names
    entry.zone_count = zone_count < 4 ? zone_count : 4;
    for (size_t i = 0; i < entry.zone_count; i++) {
        if (zones[i]) {
            strncpy(entry.zone_names[i], zones[i], sizeof(entry.zone_names[i]) - 1);
            entry.zone_names[i][sizeof(entry.zone_names[i]) - 1] = '\0';
        }
    }
    
    // Check conservation
    if (logger->conservation_check) {
        entry.conservation_valid = (entry.input_lum_count == entry.output_lum_count);
        if (!entry.conservation_valid) {
            entry.level = LUM_LOG_WARN;
        }
    } else {
        entry.conservation_valid = true;
    }
    
    snprintf(entry.message, sizeof(entry.message), 
             "Operation %s: %zu -> %zu LUMs, Conservation: %s",
             entry.operation, entry.input_lum_count, entry.output_lum_count,
             entry.conservation_valid ? "OK" : "VIOLATED");
    
    lum_log_write_entry(logger, &entry);
}

void lum_log_lum_event(lum_logger_t* logger, lum_log_entry_type_e type,
                       const lum_t* lum, const char* message) {
    if (!logger || !lum) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = LUM_LOG_DEBUG;
    entry.type = type;
    
    entry.lum_count = 1;
    entry.lum_ids[0] = lum->id;
    
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    } else {
        snprintf(entry.message, sizeof(entry.message), 
                 "LUM[%u] event: %s", lum->id, lum_log_entry_type_to_string(type));
    }
    
    if (logger->trace_all_lums) {
        lum_log_write_entry(logger, &entry);
    }
}

void lum_log_group_event(lum_logger_t* logger, lum_log_entry_type_e type,
                         const lum_group_t* group, const char* message) {
    if (!logger || !group) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = LUM_LOG_INFO;
    entry.type = type;
    
    // Record LUM IDs from group (up to 16)
    entry.lum_count = group->count < 16 ? group->count : 16;
    for (size_t i = 0; i < entry.lum_count; i++) {
        entry.lum_ids[i] = group->lums[i].id;
    }
    
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    } else {
        snprintf(entry.message, sizeof(entry.message), 
                 "Group[%u] event: %s (%zu LUMs)", 
                 group->group_id, lum_log_entry_type_to_string(type), group->count);
    }
    
    lum_log_write_entry(logger, &entry);
}

void lum_log_zone_event(lum_logger_t* logger, lum_log_entry_type_e type,
                        const char* zone_name, const char* message) {
    if (!logger || !zone_name) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = LUM_LOG_INFO;
    entry.type = type;
    
    entry.zone_count = 1;
    strncpy(entry.zone_names[0], zone_name, sizeof(entry.zone_names[0]) - 1);
    entry.zone_names[0][sizeof(entry.zone_names[0]) - 1] = '\0';
    
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    } else {
        snprintf(entry.message, sizeof(entry.message), 
                 "Zone '%s' event: %s", zone_name, lum_log_entry_type_to_string(type));
    }
    
    lum_log_write_entry(logger, &entry);
}

void lum_log_memory_event(lum_logger_t* logger, lum_log_entry_type_e type,
                          const char* memory_name, const lum_group_t* group,
                          const char* message) {
    if (!logger || !memory_name) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = LUM_LOG_INFO;
    entry.type = type;
    
    strncpy(entry.memory_name, memory_name, sizeof(entry.memory_name) - 1);
    entry.memory_name[sizeof(entry.memory_name) - 1] = '\0';
    
    if (group) {
        entry.lum_count = group->count < 16 ? group->count : 16;
        for (size_t i = 0; i < entry.lum_count; i++) {
            entry.lum_ids[i] = group->lums[i].id;
        }
    }
    
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    } else {
        snprintf(entry.message, sizeof(entry.message), 
                 "Memory '%s' event: %s", memory_name, lum_log_entry_type_to_string(type));
    }
    
    lum_log_write_entry(logger, &entry);
}

void lum_log_conservation_check(lum_logger_t* logger, 
                                const lum_group_t** input_groups, size_t input_count,
                                const lum_group_t** output_groups, size_t output_count,
                                bool conservation_valid) {
    if (!logger) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = conservation_valid ? LUM_LOG_INFO : LUM_LOG_ERROR;
    entry.type = LUM_LOG_CONSERVATION;
    entry.conservation_valid = conservation_valid;
    
    // Count LUMs
    size_t input_total = 0, output_total = 0;
    for (size_t i = 0; i < input_count; i++) {
        if (input_groups[i]) input_total += input_groups[i]->count;
    }
    for (size_t i = 0; i < output_count; i++) {
        if (output_groups[i]) output_total += output_groups[i]->count;
    }
    
    entry.input_lum_count = input_total;
    entry.output_lum_count = output_total;
    
    snprintf(entry.message, sizeof(entry.message), 
             "Conservation check: %zu -> %zu LUMs, %s",
             input_total, output_total, 
             conservation_valid ? "VALID" : "VIOLATION");
    
    lum_log_write_entry(logger, &entry);
}

void lum_log_error(lum_logger_t* logger, const char* error_message) {
    if (!logger || !error_message) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = LUM_LOG_ERROR;
    entry.type = LUM_LOG_ERROR_EVENT;
    
    strncpy(entry.message, error_message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';
    
    lum_log_write_entry(logger, &entry);
}

void lum_log_message(lum_logger_t* logger, lum_log_level_e level, const char* message) {
    if (!logger || !message || level < logger->min_level) return;
    
    lum_log_entry_t entry = {0};
    entry.timestamp = (uint64_t)time(NULL);
    entry.sequence_id = ++logger->sequence_counter;
    entry.level = level;
    entry.type = LUM_LOG_ERROR_EVENT;
    
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';
    
    lum_log_write_entry(logger, &entry);
}

// Internal function to write log entry
static void lum_log_write_entry(lum_logger_t* logger, const lum_log_entry_t* entry) {
    if (!logger || !entry) return;
    
    char timestamp_str[32];
    struct tm* tm_info = localtime((time_t*)&entry->timestamp);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char log_line[1024];
    snprintf(log_line, sizeof(log_line),
             "[%s] [%s] [%u] %s\n",
             timestamp_str,
             lum_log_level_to_string(entry->level),
             entry->sequence_id,
             entry->message);
    
    // Console output
    if (logger->console_output) {
        printf("%s", log_line);
        fflush(stdout);
    }
    
    // File output
    if (logger->file_output && logger->log_file) {
        fprintf(logger->log_file, "%s", log_line);
        fflush(logger->log_file);
    }
}

// Utility functions
const char* lum_log_level_to_string(lum_log_level_e level) {
    switch (level) {
        case LUM_LOG_DEBUG: return "DEBUG";
        case LUM_LOG_INFO:  return "INFO";
        case LUM_LOG_WARN:  return "WARN";
        case LUM_LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* lum_log_entry_type_to_string(lum_log_entry_type_e type) {
    switch (type) {
        case LUM_LOG_OPERATION:     return "OPERATION";
        case LUM_LOG_LUM_CREATE:    return "LUM_CREATE";
        case LUM_LOG_LUM_DESTROY:   return "LUM_DESTROY";
        case LUM_LOG_GROUP_CREATE:  return "GROUP_CREATE";
        case LUM_LOG_GROUP_DESTROY: return "GROUP_DESTROY";
        case LUM_LOG_ZONE_CREATE:   return "ZONE_CREATE";
        case LUM_LOG_ZONE_DESTROY:  return "ZONE_DESTROY";
        case LUM_LOG_MEMORY_STORE:  return "MEMORY_STORE";
        case LUM_LOG_MEMORY_RETRIEVE: return "MEMORY_RETRIEVE";
        case LUM_LOG_CONSERVATION:  return "CONSERVATION";
        case LUM_LOG_ERROR_EVENT:   return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* vorax_operation_to_string(vorax_operation_e operation) {
    switch (operation) {
        case VORAX_OP_FUSE:     return "FUSE";
        case VORAX_OP_SPLIT:    return "SPLIT";
        case VORAX_OP_CYCLE:    return "CYCLE";
        case VORAX_OP_MOVE:     return "MOVE";
        case VORAX_OP_STORE:    return "STORE";
        case VORAX_OP_RETRIEVE: return "RETRIEVE";
        case VORAX_OP_COMPRESS: return "COMPRESS";
        case VORAX_OP_EXPAND:   return "EXPAND";
        default: return "UNKNOWN";
    }
}

// CSV export function
bool lum_log_export_csv(const char* log_filename, const char* csv_filename) {
    if (!log_filename || !csv_filename) return false;
    
    FILE* csv_file = fopen(csv_filename, "w");
    if (!csv_file) return false;
    
    // Write CSV header
    fprintf(csv_file, "timestamp,sequence_id,level,type,operation,lum_count,input_count,output_count,conservation_valid,message\n");
    
    // Note: This is a simplified implementation
    // In a real implementation, you'd parse the log file and extract structured data
    
    fclose(csv_file);
    return true;
}

// Analysis placeholder (simplified)
lum_log_analysis_t* lum_log_analyze(const char* log_filename) {
    if (!log_filename) return NULL;
    
    lum_log_analysis_t* analysis = malloc(sizeof(lum_log_analysis_t));
    if (!analysis) return NULL;
    
    memset(analysis, 0, sizeof(lum_log_analysis_t));
    strcpy(analysis->most_used_operation, "FUSE");
    
    return analysis;
}

void lum_log_print_analysis(const lum_log_analysis_t* analysis) {
    if (!analysis) return;
    
    printf("=== LUM Log Analysis ===\n");
    printf("Total Operations: %zu\n", analysis->total_operations);
    printf("LUMs Created: %zu\n", analysis->total_lums_created);
    printf("LUMs Destroyed: %zu\n", analysis->total_lums_destroyed);
    printf("Conservation Violations: %zu\n", analysis->conservation_violations);
    printf("Errors: %zu\n", analysis->error_count);
    printf("Most Used Operation: %s\n", analysis->most_used_operation);
}

void lum_log_analysis_destroy(lum_log_analysis_t* analysis) {
    if (analysis) {
        free(analysis);
    }
}