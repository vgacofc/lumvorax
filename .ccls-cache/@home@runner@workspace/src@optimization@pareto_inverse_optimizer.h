
#ifndef PARETO_INVERSE_OPTIMIZER_H
#define PARETO_INVERSE_OPTIMIZER_H

#include "../lum/lum_core.h"
#include "pareto_optimizer.h"
#include <time.h>

// Types d'optimisation par couche
typedef enum {
    OPT_TYPE_MEMORY = 0,      // Optimisation mémoire avec pooling
    OPT_TYPE_SIMD = 1,        // Optimisation SIMD vectorielle
    OPT_TYPE_PARALLEL = 2,    // Optimisation parallèle multi-thread
    OPT_TYPE_CRYPTO = 3,      // Optimisation cryptographique matérielle
    OPT_TYPE_ENERGY = 4       // Optimisation énergétique
} optimization_type_e;

// Couche d'optimisation spécialisée
typedef struct {
    char name[64];
    optimization_type_e type;
    double efficiency_target;
    double current_efficiency;
    bool active;
    double execution_time;      // Temps d'exécution de cette couche
    double energy_consumption;  // Consommation énergétique
    double memory_footprint;    // Empreinte mémoire
} optimization_layer_t;

// Optimiseur Pareto inversé multi-couches
typedef struct {
    optimization_layer_t* layers;
    size_t layer_count;
    size_t max_layers;
    bool inverse_mode_active;
    double global_efficiency_target;
    double energy_budget;
    char multi_layer_script[4096];  // Script DSL VORAX multi-couches
} pareto_inverse_optimizer_t;

// Résultat d'optimisation multi-couches
typedef struct {
    bool success;
    char error_message[256];
    char summary[512];
    pareto_metrics_t baseline_metrics;
    pareto_metrics_t final_metrics;
    double inverse_pareto_score;
    double total_improvement;      // Amélioration totale en pourcentage
    double total_execution_time;   // Temps total d'optimisation
    lum_group_t* optimized_group;  // Groupe résultant optimisé
} pareto_inverse_result_t;

// Fonctions principales
pareto_inverse_optimizer_t* pareto_inverse_optimizer_create(void);
void pareto_inverse_optimizer_destroy(pareto_inverse_optimizer_t* optimizer);

// Gestion des couches d'optimisation
optimization_layer_t* pareto_add_optimization_layer(pareto_inverse_optimizer_t* optimizer, 
                                                   const char* layer_name,
                                                   optimization_type_e type,
                                                   double efficiency_target);

// Exécution optimisation multi-couches
pareto_inverse_result_t pareto_execute_multi_layer_optimization(pareto_inverse_optimizer_t* optimizer,
                                                               lum_group_t* input_group);

// Calcul score Pareto inversé avancé
double calculate_inverse_pareto_score_advanced(const pareto_metrics_t* optimized, 
                                             const pareto_metrics_t* baseline);

// Fonctions d'optimisation spécialisées par couche
lum_group_t* apply_memory_optimization(lum_group_t* group, optimization_layer_t* layer);
lum_group_t* apply_simd_optimization(lum_group_t* group, optimization_layer_t* layer);
lum_group_t* apply_parallel_optimization(lum_group_t* group, optimization_layer_t* layer);
lum_group_t* apply_crypto_optimization(lum_group_t* group, optimization_layer_t* layer);
lum_group_t* apply_energy_optimization(lum_group_t* group, optimization_layer_t* layer);

// Génération de rapports
void pareto_generate_multi_layer_report(pareto_inverse_optimizer_t* optimizer, 
                                       const pareto_inverse_result_t* result,
                                       const char* output_file);

#endif // PARETO_INVERSE_OPTIMIZER_H
