#ifndef PARETO_OPTIMIZER_H
#define PARETO_OPTIMIZER_H

#include "../lum/lum_core.h"
#include "../vorax/vorax_operations.h"
#include "../parser/vorax_parser.h"

// Types pour l'optimisation Pareto
typedef struct {
    double efficiency_ratio;     // Ratio efficacité/coût
    double memory_usage;         // Usage mémoire en bytes
    double execution_time;       // Temps d'exécution en microsecondes
    double energy_consumption;   // Consommation énergétique estimée
    size_t lum_operations_count; // Nombre d'opérations LUM
} pareto_metrics_t;

// Point Pareto dans l'espace multidimensionnel
typedef struct {
    pareto_metrics_t metrics;
    bool is_dominated;           // True si dominé par un autre point
    double pareto_score;         // Score Pareto inversé (plus haut = meilleur)
    char optimization_path[512]; // Chemin d'optimisation appliqué
} pareto_point_t;

// Optimiseur Pareto principal
typedef struct {
    pareto_point_t* points;
    size_t point_count;
    size_t point_capacity;
    pareto_point_t current_best;
    bool inverse_pareto_mode;    // Mode Pareto inversé activé
    char vorax_optimization_script[2048]; // Script VORAX pour optimisations
} pareto_optimizer_t;

// Configuration pour optimisations Pareto
typedef struct {
    bool enable_simd_optimization;
    bool enable_memory_pooling;
    bool enable_parallel_processing;
    size_t max_optimization_layers;
    size_t max_points;  // Capacité maximale de points Pareto
} pareto_config_t;

// Fonctions principales
pareto_optimizer_t* pareto_optimizer_create(const pareto_config_t* config);
void pareto_optimizer_destroy(pareto_optimizer_t* optimizer);

// Évaluation des métriques
pareto_metrics_t pareto_evaluate_metrics(lum_group_t* group, const char* operation_sequence);
bool pareto_is_dominated(const pareto_metrics_t* a, const pareto_metrics_t* b);
double pareto_calculate_inverse_score(const pareto_metrics_t* metrics);

// Optimisations spécifiques
vorax_result_t* pareto_optimize_fuse_operation(lum_group_t* group1, lum_group_t* group2);
vorax_result_t* pareto_optimize_split_operation(lum_group_t* group, size_t parts);
vorax_result_t* pareto_optimize_cycle_operation(lum_group_t* group, size_t modulo);

// DSL VORAX pour optimisations
bool pareto_execute_vorax_optimization(pareto_optimizer_t* optimizer, const char* vorax_script);
char* pareto_generate_optimization_script(const pareto_metrics_t* target_metrics);

// Fonctions utilitaires
void pareto_add_point(pareto_optimizer_t* optimizer, const pareto_metrics_t* metrics, const char* path);
pareto_point_t* pareto_find_best_point(pareto_optimizer_t* optimizer);
void pareto_update_dominance(pareto_optimizer_t* optimizer);

// Benchmarking et comparaison
void pareto_benchmark_against_baseline(pareto_optimizer_t* optimizer, const char* baseline_operation);
void pareto_generate_performance_report(pareto_optimizer_t* optimizer, const char* output_file);

#endif // PARETO_OPTIMIZER_H