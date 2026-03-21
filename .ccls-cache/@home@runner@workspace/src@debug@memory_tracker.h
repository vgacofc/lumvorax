#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h> // Include for bool type
#include <stdint.h>  // CORRECTION: Include pour uint64_t

// Configuration du debugging mémoire
#define MEMORY_DEBUG_ENABLED 1
#define MAX_MEMORY_ENTRIES 50000

typedef struct {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
    time_t allocated_time;
    int is_freed;
    time_t freed_time;
    const char* freed_file;
    int freed_line;
    const char* freed_function;
    uint64_t generation;  // CORRECTION: Gestion génération pour réutilisation pointeurs
} memory_entry_t;

typedef struct {
    memory_entry_t entries[MAX_MEMORY_ENTRIES];
    size_t count;
    size_t total_allocated;
    size_t total_freed;
    size_t peak_usage;
    size_t current_usage;
} memory_tracker_t;

// Macros pour traçage automatique
#define TRACKED_MALLOC(size) tracked_malloc(size, __FILE__, __LINE__, __func__)
#define TRACKED_FREE(ptr) tracked_free(ptr, __FILE__, __LINE__, __func__)
#define TRACKED_CALLOC(nmemb, size) tracked_calloc(nmemb, size, __FILE__, __LINE__, __func__)
#define TRACKED_REALLOC(ptr, size) tracked_realloc(ptr, size, __FILE__, __LINE__, __func__)
#define TRACKED_STRDUP(str) tracked_strdup(str, __FILE__, __LINE__, __func__)  // NOUVEAU
#define TRACKED_STRNDUP(str, n) tracked_strndup(str, n, __FILE__, __LINE__, __func__)  // NOUVEAU

// Fonctions publiques
void memory_tracker_init(void);
void memory_tracker_cleanup(void);
void memory_tracker_check_leaks(void);
void memory_tracker_destroy(void);
void memory_tracker_alloc(void* ptr, size_t size, const char* file, int line);
void memory_tracker_free(void* ptr, const char* file, int line);
void memory_tracker_report(void);

// Fonctions tracked pour macros
void* tracked_malloc(size_t size, const char* file, int line, const char* func);
void tracked_free(void* ptr, const char* file, int line, const char* func);
void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* func);
void* tracked_realloc(void* ptr, size_t size, const char* file, int line, const char* func);
char* tracked_strdup(const char* str, const char* file, int line, const char* func);  // NOUVEAU
char* tracked_strndup(const char* str, size_t n, const char* file, int line, const char* func);  // NOUVEAU

// Contrôle runtime tracking
void memory_tracker_enable(bool enable);
bool memory_tracker_is_enabled(void);
void memory_tracker_export_json(const char* filename);
void memory_tracker_set_release_mode(bool release_mode);
void memory_tracker_cleanup(void);
size_t memory_tracker_get_current_usage(void);


#endif // MEMORY_TRACKER_H