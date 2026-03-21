
#ifndef SIMD_OPTIMIZER_H
#define SIMD_OPTIMIZER_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __AVX512F__
#include <immintrin.h>
#define SIMD_VECTOR_SIZE 16  // 512 bits / 32 bits per int
#elif __AVX2__
#include <immintrin.h>
#define SIMD_VECTOR_SIZE 8   // 256 bits / 32 bits per int
#else
#define SIMD_VECTOR_SIZE 1   // Fallback to scalar
#endif

// SIMD optimization configuration
typedef struct {
    bool avx512_available;
    bool avx512_supported;     // Ajouté pour compatibilité
    bool has_avx512;           // Compatibilité tests individuels
    bool avx2_available;
    bool avx2_supported;       // Ajouté pour compatibilité
    bool has_avx2;             // Compatibilité tests individuels
    bool has_avx;              // Compatibilité tests individuels
    bool sse_available;
    bool sse42_supported;      // Ajouté pour compatibilité
    bool has_sse;              // Compatibilité tests individuels
    bool has_sse2;             // Compatibilité tests individuels
    int vector_width;
    char cpu_features[256];
} simd_capabilities_t;

// SIMD optimizer configuration
typedef struct {
    simd_capabilities_t capabilities;
    bool initialized;
    char processor_info[256];
} simd_optimizer_t;

// SIMD operation types
typedef enum {
    SIMD_VECTOR_ADD,
    SIMD_VECTOR_MULTIPLY,
    SIMD_PARALLEL_TRANSFORM,
    SIMD_FUSED_MULTIPLY_ADD
} simd_operation_e;

// SIMD operation results
typedef struct {
    bool success;              // Indicateur de succès (compatibilité tests)
    size_t processed_elements;
    size_t elements_processed; // Alias pour processed_elements (compatibilité tests)
    double execution_time;
    double throughput_ops_per_sec;
    bool used_vectorization;
    char optimization_used[64];
    size_t vectorized_count;
    size_t scalar_fallback_count;
    double performance_gain;
    double acceleration_factor; // Alias pour performance_gain (compatibilité tests)
    uint64_t execution_time_ns;
} simd_result_t;

// Function declarations
simd_capabilities_t* simd_detect_capabilities(void);
void simd_capabilities_destroy(simd_capabilities_t* caps);

// Vectorized LUM operations
simd_result_t* simd_process_lum_array_bulk(lum_t* lums, size_t count);
simd_result_t* simd_fuse_operations_bulk(lum_group_t** groups, size_t group_count);
simd_result_t* simd_binary_conversion_bulk(uint8_t* data, size_t data_size);

// AVX2 specific implementations
#ifdef __AVX2__
void simd_avx2_process_presence_bits(uint32_t* presence_array, size_t count);
void simd_avx2_parallel_coordinate_transform(float* x_coords, float* y_coords, size_t count);
#endif

// AVX-512 specific implementations  
#ifdef __AVX512F__
void simd_avx512_mass_lum_operations_void(lum_t* lums, size_t count);
void simd_avx512_vectorized_conservation_check(uint64_t* conservation_data, size_t count);
#endif

// Version avec retour pour tests
simd_result_t* simd_avx512_mass_lum_operations(lum_t* lums, size_t count);

// Fonction wrapper pour compatibilité tests avec float*
simd_result_t* simd_process_float_array_bulk(float* array, size_t count);

// Performance benchmarking
simd_result_t* simd_benchmark_vectorization(size_t test_size);
void simd_result_destroy(simd_result_t* result);
void simd_print_performance_comparison(simd_result_t* scalar, simd_result_t* vectorized);

// SIMD optimization functions
bool simd_optimize_lum_operations(simd_optimizer_t* optimizer, 
                                   lum_group_t* group, 
                                   simd_operation_e operation,
                                   simd_result_t* result);

// Operation-specific functions
bool simd_vector_add_lums(simd_optimizer_t* optimizer, lum_group_t* group, simd_result_t* result);
bool simd_vector_multiply_lums(simd_optimizer_t* optimizer, lum_group_t* group, simd_result_t* result);
bool simd_parallel_transform_lums(simd_optimizer_t* optimizer, lum_group_t* group, simd_result_t* result);
bool simd_fma_lums(simd_optimizer_t* optimizer, lum_group_t* group, simd_result_t* result);

#endif // SIMD_OPTIMIZER_H
