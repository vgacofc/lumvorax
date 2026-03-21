
#ifndef ENHANCED_LOGGING_H
#define ENHANCED_LOGGING_H

// Système de logs avancé conforme rapport 091
void create_log_directory(const char* base_path);
void create_enhanced_log(const char* filepath, const char* message);
void log_module_action_enhanced(const char* module_name, const char* action);
void log_test_execution(const char* test_name, const char* result, int line_count);

// Macros pour faciliter l'usage
#define LOG_MODULE_ACTION(module, action) \
    log_module_action_enhanced(module, action)

#define LOG_TEST_RESULT(test, result, lines) \
    log_test_execution(test, result, lines)

#endif // ENHANCED_LOGGING_H
