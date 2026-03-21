#ifndef RSA_STRUCTURE_ANALYZER_H
#define RSA_STRUCTURE_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <complex.h>

#define MAX_EXPERIMENT_NAME 128

typedef enum {
    OBJ_GENERIC,
    OBJ_COMPOSITE_NON_RSA,
    OBJ_PRIME_ISOLATED,
    OBJ_RSA_LIKE
} object_type_e;

typedef struct {
    uint64_t n;
    object_type_e type;
    size_t bit_size;
} study_object_t;

typedef struct {
    double scale;
    double coprimality_density;
    double residual_variance;
    double complex spectral_signature;
    double local_fluctuation;
} local_observable_t;

typedef struct {
    study_object_t object;
    local_observable_t* observables;
    size_t observable_count;
    uint32_t checksum;
    char timestamp[32];
} experiment_result_t;

// Core Analysis Functions
local_observable_t calculate_local_observable(uint64_t n, double scale);
bool run_differential_analysis(const experiment_result_t* baseline, const experiment_result_t* rsa_like, size_t count);
void generate_forensic_report(const char* filename);

#endif
