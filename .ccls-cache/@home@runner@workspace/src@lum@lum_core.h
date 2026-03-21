#ifndef LUM_CORE_H_INCLUDED
#define LUM_CORE_H_INCLUDED

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI
// Aucune utilisation d'emoji dans le code source ou dans les fichiers de log. 
// Toute inclusion d'emoji sera considérée comme une violation des standards de codage.
// Cette règle s'applique à TOUS les modules du système LUM/VORAX sans exception.

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

// Vérification de l'ABI - la structure lum_t réelle avec tous ses champs

// Core LUM structure - a single presence unit (ALIGNÉ 64 BYTES POUR CACHE/SIMD)
typedef struct {
    uint32_t id;                    // Identifiant unique
    uint8_t presence;               // État de présence (0 ou 1)
    uint8_t structure_type;         // Type de LUM (conforme STANDARD_NAMES)
    uint8_t is_destroyed;           // Protection double-free (nouveau STANDARD_NAMES 2025-01-10)
    uint8_t reserved_flags;         // Flags réservés
    int32_t position_x;             // Position spatiale X (conforme STANDARD_NAMES)
    int32_t position_y;             // Position spatiale Y (conforme STANDARD_NAMES)
    uint64_t timestamp;             // Timestamp de création nanoseconde
    void* memory_address;           // Adresse mémoire pour traçabilité
    uint32_t checksum;              // Vérification intégrité
    uint32_t magic_number;          // Magic number pour validation ultra-sécurisée
    uint8_t padding[20];            // Padding étendu pour alignement 64 bytes total
} lum_t;

// Vérification ABI corrigée - alignement 64 bytes pour performance cache/SIMD
_Static_assert(sizeof(lum_t) == 64, "lum_t structure must be exactly 64 bytes for cache line alignment");

// LUM structure types
typedef enum {
    LUM_STRUCTURE_LINEAR = 0,
    LUM_STRUCTURE_CIRCULAR = 1,
    LUM_STRUCTURE_BINARY = 2,
    LUM_STRUCTURE_GROUP = 3,
    LUM_STRUCTURE_COMPRESSED = 4,
    LUM_STRUCTURE_NODE = 5,
    LUM_STRUCTURE_MAX = 6
} lum_structure_type_e;

// Allocation tracking for forensic memory management
typedef enum {
    LUM_ALLOC_TRACKED = 0,    // TRACKED_MALLOC - use TRACKED_FREE
    LUM_ALLOC_ALIGNED = 1,    // aligned_alloc - use free()
    LUM_ALLOC_MMAP = 2        // mmap - use munmap()
} lum_allocation_method_e;

// LUM Group - collection of LUMs
typedef struct {
    lum_t* lums;              // Array of LUMs (stockage par valeur)
    size_t count;             // Number of LUMs
    size_t capacity;          // Allocated capacity
    uint32_t group_id;        // Group identifier
    lum_structure_type_e type; // Group structure type
    uint32_t magic_number;    // Protection double-free (nouveau STANDARD_NAMES 2025-01-10)
    lum_allocation_method_e alloc_method; // Track allocation method for correct deallocation
    size_t allocated_size;    // Size for munmap() if needed
} lum_group_t;

// Zone - spatial container for LUMs
typedef struct {
    char name[32];            // Zone name (A, B, C, etc.)
    lum_group_t** groups;     // Array of pointers to LUM groups
    size_t group_count;       // Number of groups
    size_t group_capacity;    // Allocated capacity for groups
    bool is_empty;            // Quick empty check
} lum_zone_t;

// Memory storage for LUMs
typedef struct {
    char name[32];            // Memory variable name (#alpha, #beta, etc.)
    lum_group_t stored_group; // Stored group
    bool is_occupied;         // Whether memory contains data
} lum_memory_t;

// Core functions
lum_t* lum_create(uint8_t presence, int32_t x, int32_t y, lum_structure_type_e type);
void lum_destroy(lum_t* lum);

lum_group_t* lum_group_create(size_t initial_capacity);
void lum_group_destroy(lum_group_t* group);
void lum_group_safe_destroy(lum_group_t** group_ptr);
bool lum_group_add(lum_group_t* group, lum_t* lum);
lum_t* lum_group_get(lum_group_t* group, size_t index);
size_t lum_group_size(lum_group_t* group);

lum_zone_t* lum_zone_create(const char* name);
void lum_zone_destroy(lum_zone_t* zone);
bool lum_zone_add_group(lum_zone_t* zone, lum_group_t* group);
bool lum_zone_is_empty(lum_zone_t* zone);

lum_memory_t* lum_memory_create(const char* name);
void lum_memory_destroy(lum_memory_t* memory);
bool lum_memory_store(lum_memory_t* memory, lum_group_t* group);
lum_group_t* lum_memory_retrieve(lum_memory_t* memory);

// OPTIMISATIONS AVANCÉES: Types pour opérations batch
typedef enum {
    LUM_BATCH_VALIDATE_ALL = 0,
    LUM_BATCH_UPDATE_TIMESTAMPS = 1,
    LUM_BATCH_RECALC_CHECKSUMS = 2,
    LUM_BATCH_SORT_BY_ID = 3,
    LUM_BATCH_DEFRAGMENT = 4
} lum_batch_operation_e;

// Utility functions
uint32_t lum_generate_id(void);
uint64_t lum_get_timestamp(void);
void lum_print(const lum_t* lum);
void lum_group_print(const lum_group_t* group);

// OPTIMISATIONS AVANCÉES: Fonctions batch ultra-optimisées 50M+ LUMs
bool lum_group_process_batch_50m_optimized(lum_group_t* group, lum_batch_operation_e operation);
bool lum_group_sort_ultra_fast(lum_group_t* group);
bool lum_group_defragment_zero_copy(lum_group_t* group);

// Fonction de destruction sécurisée
void lum_safe_destroy(lum_t** lum_ptr);

// PRIORITÉ 1.2: Fonction destruction groupe ultra-sécurisée selon roadmap
void lum_group_destroy_ultra_secure(lum_group_t** group_ptr);

// Constantes de validation mémoire - SÉCURISÉES CONFORMES RAPPORT 113
// CORRECTION CRITIQUE: Magic numbers générés cryptographiquement
// CORRECTION RAPPORT 129 ANOMALIE #001: Magic numbers centralisés
#include "../common/magic_numbers.h"

extern uint32_t LUM_VALIDATION_PATTERN; // Unifié = LUM_CORE_MAGIC

// Compatibilité avec ancien code - aliases vers magic numbers unifiés
#define LUM_MAGIC_DESTROYED MAGIC_DESTROYED_PATTERN
#define LUM_DESTROYED_MAGIC MAGIC_DESTROYED_PATTERN

// Fonctions d'initialisation et nettoyage sécurisés
bool lum_security_init(void);
void lum_security_cleanup(void);

// Macro de validation magic number
#define VALIDATE_LUM_MAGIC(lum) \
    do { \
        if (!(lum) || (lum)->magic_number != LUM_MAGIC_NUMBER) { \
            forensic_log(FORENSIC_LEVEL_ERROR, __func__, \
                        "Invalid LUM magic: %p (magic: 0x%08X)", \
                        (void*)(lum), (lum) ? (lum)->magic_number : 0); \
            return false; \
        } \
    } while(0)

#define VALIDATE_LUM_MAGIC_PTR(lum) \
    do { \
        if (!(lum) || (lum)->magic_number != LUM_MAGIC_NUMBER) { \
            forensic_log(FORENSIC_LEVEL_ERROR, __func__, \
                        "Invalid LUM magic: %p (magic: 0x%08X)", \
                        (void*)(lum), (lum) ? (lum)->magic_number : 0); \
            return NULL; \
        } \
    } while(0)

// PRIORITÉ 1.3: TIMING FORENSIQUE DIFFÉRENCIÉ selon roadmap exact
// LOGS GRANULAIRES: CLOCK_MONOTONIC pour mesures précises
#define FORENSIC_TIMING_START(timer_var) \
    struct timespec timer_var##_start, timer_var##_end; \
    clock_gettime(CLOCK_MONOTONIC, &timer_var##_start)

#define FORENSIC_TIMING_END(timer_var) \
    clock_gettime(CLOCK_MONOTONIC, &timer_var##_end)

#define FORENSIC_TIMING_CALC_NS(timer_var) \
    ((timer_var##_end.tv_sec - timer_var##_start.tv_sec) * 1000000000ULL + \
     (timer_var##_end.tv_nsec - timer_var##_start.tv_nsec))

// FICHIERS/MÉTADONNÉES: CLOCK_REALTIME pour horodatage
#define FILE_TIMESTAMP_GET() \
    ({ \
        struct timespec ts; \
        clock_gettime(CLOCK_REALTIME, &ts); \
        ts.tv_sec * 1000000000ULL + ts.tv_nsec; \
    })

// PRIORITÉ 2.2: GESTION ERREURS ZERO-TOLERANCE selon roadmap exact
typedef struct {
    bool success;
    char error_message[256];
    void* data;
    uint32_t error_code;
} result_t;

#define LUM_ERROR_CODE_FORENSIC 1 // Renommé pour éviter conflit enum
#define LUM_ERROR_CODE_CRITICAL 2 // Renommé pour éviter conflit enum

// Pattern obligatoire zero-tolerance
#define CHECK_RESULT_OR_FAIL(result, cleanup_call, error_msg) \
    do { \
        if (!(result).success) { \
            printf("[LUM_ERROR_CODE_FORENSIC] %s failed: %s\n", #result, (result).error_message); \
            cleanup_call; \
            printf("[LUM_ERROR_CODE_FORENSIC] Chain failure: %s\n", error_msg); \
            return (result_t){false, error_msg, NULL, 1}; \
        } \
    } while(0)

// PRIORITÉ 2.3: VALIDATION RANGES SYSTÉMATIQUE selon roadmap exact - CORRECTIONS APPLIQUÉES
#define VALIDATE_ARRAY_ACCESS(array, index, size, context) \
    do { \
        if ((index) >= (size)) { \
            printf("[FORENSIC_CRITICAL] Array access out of bounds in %s: index=%zu size=%zu\n", \
                (context), (size_t)(index), (size_t)(size)); \
            abort(); \
        } \
    } while(0)

// NOUVELLE: Validation pointeur non-null
#define VALIDATE_NON_NULL(ptr, context) \
    do { \
        if (!(ptr)) { \
            printf("[FORENSIC_CRITICAL] NULL pointer access in %s\n", (context)); \
            abort(); \
        } \
    } while(0)

// NOUVELLE: Validation magic number
#define VALIDATE_MAGIC_NUMBER(ptr, expected, context) \
    do { \
        if ((ptr)->magic_number != (expected)) { \
            printf("[FORENSIC_CRITICAL] Invalid magic number in %s: got=0x%X expected=0x%X\n", \
                (context), (ptr)->magic_number, (expected)); \
            abort(); \
        } \
    } while(0)

// Macros de validation
#define VALIDATE_LUM_PTR(ptr) \
    do { \
        if (!(ptr)) { \
            printf("ERROR: NULL LUM pointer at %s:%d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#define VALIDATE_GROUP_PTR(ptr) \
    do { \
        if (!(ptr)) { \
            printf("ERROR: NULL Group pointer at %s:%d\n", __FILE__, __LINE__); \
            return false; \
        } \
        if ((ptr)->magic_number == MAGIC_DESTROYED_PATTERN) { \
            printf("ERROR: Use of destroyed group at %s:%d\n", __FILE__, __LINE__); \
            return false; \
        } \
        if ((ptr)->magic_number != LUM_VALIDATION_PATTERN) { \
            printf("ERROR: Corrupted group (magic=0x%X) at %s:%d\n", (ptr)->magic_number, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

#endif /* LUM_CORE_H_INCLUDED */ // LUM_CORE_H