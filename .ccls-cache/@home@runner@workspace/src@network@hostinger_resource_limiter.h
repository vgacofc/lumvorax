
#ifndef HOSTINGER_RESOURCE_LIMITER_H
#define HOSTINGER_RESOURCE_LIMITER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Fonctions vérification ressources serveur Hostinger
bool hostinger_check_cpu_availability(void);
bool hostinger_check_ram_availability(size_t required_mb);
bool hostinger_check_lum_processing_limit(size_t lum_count);

// Gestion monitor ressources
// Structure complète pour moniteur ressources Hostinger
typedef struct hostinger_resource_monitor_t {
    uint64_t current_ram_usage_mb;      // RAM utilisée en MB
    uint64_t max_ram_allowed_mb;        // Limite RAM autorisée
    uint64_t current_cpu_percentage;    // CPU utilisé en %
    uint64_t request_count;             // Nombre requêtes traitées
    uint32_t active_threads;            // Threads actifs
    bool resource_check_enabled;       // Monitoring activé
    void* memory_address;               // Pour protection double-free
    uint32_t magic_number;              // Validation intégrité
} hostinger_resource_monitor_t;

hostinger_resource_monitor_t* hostinger_resource_monitor_create(void);
void hostinger_resource_monitor_destroy(void);

// Constantes limites serveur
#define HOSTINGER_CPU_CORES 2
#define HOSTINGER_RAM_GB 6  // Sur 7.8GB total, garder marge sécurité
#define HOSTINGER_STORAGE_GB 90  // Sur 100GB total
#define HOSTINGER_MAX_LUMS_CONCURRENT 1000000

#endif // HOSTINGER_RESOURCE_LIMITER_H
