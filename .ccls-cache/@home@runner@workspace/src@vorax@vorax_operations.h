#ifndef VORAX_OPERATIONS_H_INCLUDED
#define VORAX_OPERATIONS_H_INCLUDED

#include "../lum/lum_core.h"

// VORAX operation types
typedef enum {
    VORAX_OP_FUSE = 0,        // ⧉ - Fusion of groups
    VORAX_OP_SPLIT = 1,       // ⇅ - Split into parts
    VORAX_OP_CYCLE = 2,       // ⟲ - Cycle transformation
    VORAX_OP_MOVE = 3,        // → - Transfer between zones
    VORAX_OP_STORE = 4,       // Store in memory
    VORAX_OP_RETRIEVE = 5,    // Retrieve from memory
    VORAX_OP_COMPRESS = 6,    // Compress to Ω
    VORAX_OP_EXPAND = 7       // Expand from Ω
} vorax_operation_e;

// OPTIMISATION: Structure résultat VORAX ultra-optimisée
typedef struct {
    bool success;
    char message[256];
    lum_group_t* result_group;          // Groupe résultat principal
    lum_group_t* output_group;          // Alias compatibilité
    lum_group_t** result_groups;        // Array groupes pour split
    size_t result_count;
    double execution_time;              // Temps en secondes (legacy)
    uint64_t execution_time_ns;         // OPTIMISATION: Temps nanoseconde précis
    uint64_t operations_performed;      // OPTIMISATION: Compteur opérations
    double throughput_lums_per_sec;     // OPTIMISATION: Débit LUMs/sec
    uint32_t magic_number;              // Protection double-free
    bool use_zero_copy;                 // OPTIMISATION: Flag zero-copy utilisé
    bool use_vectorization;             // OPTIMISATION: Flag vectorisation utilisé
} vorax_result_t;

// Core VORAX operations
vorax_result_t* vorax_fuse(lum_group_t* group1, lum_group_t* group2);
vorax_result_t* vorax_split(lum_group_t* group, size_t parts);
vorax_result_t* vorax_cycle(lum_group_t* group, size_t modulo);
vorax_result_t* vorax_move(lum_zone_t* from_zone, lum_zone_t* to_zone, size_t lum_count);
vorax_result_t* vorax_store(lum_memory_t* memory, lum_zone_t* zone, size_t lum_count);
vorax_result_t* vorax_retrieve(lum_memory_t* memory, lum_zone_t* zone);
vorax_result_t* vorax_compress(lum_group_t* group);
vorax_result_t* vorax_expand(lum_group_t* compressed_group, size_t parts);

// Utility functions
vorax_result_t* vorax_result_create(void);
void vorax_result_destroy(vorax_result_t* result);
void vorax_result_safe_destroy(vorax_result_t** result_ptr);
void vorax_result_set_success(vorax_result_t* result, const char* message);
void vorax_result_set_error(vorax_result_t* result, const char* message);

// Conservation checking
bool vorax_check_conservation(lum_group_t** input_groups, size_t input_count,
                              lum_group_t** output_groups, size_t output_count);
size_t vorax_count_total_lums(lum_group_t** groups, size_t group_count);

// Advanced operations
vorax_result_t* vorax_create_node(lum_group_t* group1, lum_group_t* group2);
vorax_result_t* vorax_emit_lums(lum_zone_t* zone, size_t count);

#endif /* VORAX_OPERATIONS_H_INCLUDED */ // VORAX_OPERATIONS_H