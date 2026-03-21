#ifndef MATRIX_CALCULATOR_H
#define MATRIX_CALCULATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../lum/lum_core.h"

// Structure calculateur matriciel principal
typedef struct matrix_calculator_t {
    uint32_t magic_number;        // Protection intégrité (0x4D415452)
    size_t rows;                  // Nombre de lignes
    size_t cols;                  // Nombre de colonnes  
    double* data;                 // Données matricielles
    bool is_initialized;          // État d'initialisation
    void* memory_address;         // Protection double-free
} matrix_calculator_t;

// Structure résultat opérations matricielles
typedef struct matrix_result_t {
    uint32_t magic_number;        // Protection intégrité
    double* result_data;          // Données résultat
    size_t rows;                  // Dimensions résultat
    size_t cols;
    bool operation_success;       // Succès opération
    uint64_t execution_time_ns;   // Temps exécution
    void* memory_address;         // Protection mémoire
} matrix_result_t;

// Types unifiés pour éviter conflits
typedef struct matrix_result_t matrix_calculator_result_t; // Alias pour compatibilité

// Module de Calcul Matriciel Avancé pour LUM/VORAX
// Conforme prompt.txt - nouveau module calculs avancés

// Structure matrice LUM avec protection mémoire intégrée
typedef struct {
    lum_t** matrix_data;           // Matrice de pointeurs LUM
    size_t rows;                   // Nombre de lignes
    size_t cols;                   // Nombre de colonnes
    void* memory_address;          // Protection double-free OBLIGATOIRE
    uint32_t magic_number;         // Validation intégrité
    uint64_t timestamp;            // Horodatage création
    bool is_destroyed;             // Flag destruction
} lum_matrix_t;

// Types d'opérations matricielles
typedef enum {
    MATRIX_OP_ADD = 0,             // Addition matricielle
    MATRIX_OP_MULTIPLY = 1,        // Multiplication matricielle
    MATRIX_OP_TRANSPOSE = 2,       // Transposition
    MATRIX_OP_DETERMINANT = 3,     // Calcul déterminant
    MATRIX_OP_INVERSE = 4,         // Inversion matricielle
    MATRIX_OP_EIGENVALUES = 5,     // Valeurs propres
    MATRIX_OP_DECOMPOSE_LU = 6,    // Décomposition LU
    MATRIX_OP_SOLVE_LINEAR = 7     // Résolution système linéaire
} matrix_operation_e;

// Résultat opération matricielle LUM  
typedef struct matrix_lum_result_t {
    lum_matrix_t* result_matrix;   // Matrice résultat
    double* scalar_results;        // Résultats scalaires (déterminant, etc.)
    size_t scalar_count;           // Nombre de résultats scalaires
    bool success;                  // Succès opération
    char error_message[256];       // Message d'erreur
    uint64_t execution_time_ns;     // Temps d'exécution nanosecondes
    uint64_t operations_count;     // Nombre d'opérations effectuées
    void* memory_address;          // Protection double-free OBLIGATOIRE
} matrix_lum_result_t;

// Configuration calculs matriciels
typedef struct {
    bool use_simd_acceleration;    // Accélération SIMD
    bool use_parallel_processing;  // Traitement parallèle
    size_t thread_count;           // Nombre de threads
    double precision_threshold;    // Seuil de précision
    bool enable_caching;           // Cache résultats intermédiaires
    void* memory_address;          // Protection double-free OBLIGATOIRE
} matrix_config_t;

// Fonctions principales
matrix_calculator_t* matrix_calculator_create(size_t rows, size_t cols);
void matrix_calculator_destroy(matrix_calculator_t** calc);
void matrix_set_element(matrix_calculator_t* calc, size_t row, size_t col, double value);
matrix_result_t* matrix_multiply_lum_optimized(matrix_calculator_t* a, matrix_calculator_t* b, void* config);
void matrix_calculator_result_destroy(matrix_calculator_result_t** result_ptr);
void matrix_result_destroy(matrix_result_t** result_ptr);

lum_matrix_t* lum_matrix_create(size_t rows, size_t cols);
void lum_matrix_destroy(lum_matrix_t** matrix_ptr);
bool lum_matrix_set_lum(lum_matrix_t* matrix, size_t row, size_t col, lum_t* lum);
lum_t* lum_matrix_get_lum(lum_matrix_t* matrix, size_t row, size_t col);

// Fonctions utilitaires pour tests (valeurs simples double)
bool lum_matrix_set(lum_matrix_t* matrix, size_t row, size_t col, double value);
double lum_matrix_get(lum_matrix_t* matrix, size_t row, size_t col);

// Opérations matricielles avancées
matrix_lum_result_t* matrix_add(lum_matrix_t* matrix_a, lum_matrix_t* matrix_b, matrix_config_t* config);
matrix_lum_result_t* matrix_multiply(lum_matrix_t* matrix_a, lum_matrix_t* matrix_b, matrix_config_t* config);
matrix_lum_result_t* matrix_transpose(lum_matrix_t* matrix, matrix_config_t* config);
matrix_lum_result_t* matrix_calculate_determinant(lum_matrix_t* matrix, matrix_config_t* config);
matrix_lum_result_t* matrix_inverse(lum_matrix_t* matrix, matrix_config_t* config);
matrix_lum_result_t* matrix_eigenvalues(lum_matrix_t* matrix, matrix_config_t* config);

// Opérations spécialisées
matrix_lum_result_t* matrix_lu_decomposition(lum_matrix_t* matrix, matrix_config_t* config);
matrix_lum_result_t* matrix_solve_linear_system(lum_matrix_t* coefficient_matrix, lum_matrix_t* constants, matrix_config_t* config);

// Tests stress pour 100M+ LUMs
bool matrix_stress_test_100m_lums(matrix_config_t* config);
matrix_result_t* matrix_benchmark_operations(size_t matrix_size, matrix_config_t* config);

// Utilitaires
matrix_config_t* matrix_config_create_default(void);
void matrix_config_destroy(matrix_config_t** config_ptr);
void matrix_lum_result_destroy(matrix_lum_result_t** result_ptr);
bool matrix_validate_dimensions(lum_matrix_t* matrix_a, lum_matrix_t* matrix_b, matrix_operation_e operation);

// Constantes
#define MATRIX_MAX_SIZE 10000
#define MATRIX_MIN_DETERMINANT_THRESHOLD 1e-12
#define MATRIX_CALCULATOR_MAGIC 0x4D415452  // "MATR" en ASCII - STANDARD UNIFIÉ
#define MATRIX_MAGIC_NUMBER MATRIX_CALCULATOR_MAGIC  // Alias pour rétrocompatibilité
#define MATRIX_DESTROYED_MAGIC 0xDEADBEEF

#endif // MATRIX_CALCULATOR_H