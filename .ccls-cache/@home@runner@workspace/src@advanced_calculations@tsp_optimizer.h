
#ifndef TSP_OPTIMIZER_H
#define TSP_OPTIMIZER_H

#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Constantes TSP
#define TSP_MAX_CITIES 10000
#define TSP_MAGIC_NUMBER 0x12345678
#define TSP_DESTROYED_MAGIC 0xDEADBEEF
#define TSP_MAX_DISTANCE 999999999

// Types d'algorithmes TSP
typedef enum {
    TSP_ALGORITHM_NEAREST_NEIGHBOR,
    TSP_ALGORITHM_GENETIC,
    TSP_ALGORITHM_SIMULATED_ANNEALING,
    TSP_ALGORITHM_ANT_COLONY,
    TSP_ALGORITHM_BRANCH_BOUND,
    TSP_ALGORITHM_DYNAMIC_PROGRAMMING
} tsp_algorithm_e;

// Ville TSP basée sur LUM
typedef struct {
    lum_t base_lum;              // LUM de base
    int32_t city_id;             // ID unique de la ville
    int32_t x_coordinate;        // Coordonnée X
    int32_t y_coordinate;        // Coordonnée Y
    double cost_factor;          // Facteur de coût
    bool is_visited;             // Marqueur de visite
    uint64_t visit_timestamp;    // Timestamp de visite
    void* memory_address;        // Protection double-free
    uint32_t city_magic;         // Magic number protection
} tsp_city_t;

// Matrice de distances
typedef struct {
    size_t city_count;
    double** distance_matrix;
    uint64_t calculation_time_ns;
    void* memory_address;
    uint32_t matrix_magic;
} tsp_distance_matrix_t;

// Tour TSP
typedef struct {
    size_t city_count;
    tsp_city_t** tour_cities;
    int32_t* tour_sequence;
    double total_distance;
    bool is_valid;
    uint64_t computation_time_ns;
    void* memory_address;
    uint32_t tour_magic;
} tsp_tour_t;

// Configuration TSP
typedef struct {
    tsp_algorithm_e algorithm;
    size_t max_iterations;
    double convergence_threshold;
    bool use_parallel_processing;
    size_t thread_count;
    bool enable_caching;
    double temperature_initial;  // Pour simulated annealing
    double temperature_final;
    double cooling_rate;
    size_t population_size;      // Pour algorithme génétique
    double mutation_rate;
    double crossover_rate;
    void* memory_address;
} tsp_config_t;

// Résultat d'optimisation TSP
typedef struct {
    tsp_tour_t* optimal_tour;
    double best_distance;
    size_t iterations_performed;
    uint64_t total_time_ns;
    bool optimization_success;
    double convergence_rate;
    char algorithm_used[64];
    char error_message[256];
    void* memory_address;
} tsp_result_t;

// Fonctions principales TSP
tsp_city_t* tsp_city_create(int32_t city_id, int32_t x, int32_t y, double cost_factor);
void tsp_city_destroy(tsp_city_t** city_ptr);

tsp_distance_matrix_t* tsp_distance_matrix_create(tsp_city_t** cities, size_t city_count);
void tsp_distance_matrix_destroy(tsp_distance_matrix_t** matrix_ptr);
double tsp_calculate_distance(tsp_city_t* city1, tsp_city_t* city2);

tsp_tour_t* tsp_tour_create(size_t city_count);
void tsp_tour_destroy(tsp_tour_t** tour_ptr);
bool tsp_tour_add_city(tsp_tour_t* tour, tsp_city_t* city);
double tsp_tour_calculate_total_distance(tsp_tour_t* tour, tsp_distance_matrix_t* matrix);

// Algorithmes d'optimisation
tsp_result_t* tsp_optimize_nearest_neighbor(tsp_city_t** cities, size_t city_count, tsp_config_t* config);
tsp_result_t* tsp_optimize_genetic_algorithm(tsp_city_t** cities, size_t city_count, tsp_config_t* config);
tsp_result_t* tsp_optimize_simulated_annealing(tsp_city_t** cities, size_t city_count, tsp_config_t* config);

// Tests stress
bool tsp_stress_test_100m_cities(tsp_config_t* config);

// Configuration et nettoyage
tsp_config_t* tsp_config_create_default(void);
void tsp_config_destroy(tsp_config_t** config_ptr);
void tsp_result_destroy(tsp_result_t** result_ptr);

#endif // TSP_OPTIMIZER_H
