#ifndef AI_DYNAMIC_CONFIG_MANAGER_H
#define AI_DYNAMIC_CONFIG_MANAGER_H

#include "../lum/lum_core.h"
#include "../optimization/pareto_optimizer.h"
#include "../optimization/simd_optimizer.h"
#include "../optimization/zero_copy_allocator.h"
#include "../parallel/parallel_processor.h"
#include "../advanced_calculations/neural_network_processor.h"
// #include "../advanced_calculations/quantum_simulator.h" // DÉSACTIVÉ par prompt.txt
// #include "../crypto/homomorphic_encryption.h" // REMOVED - No homomorphic functionality
#include "../advanced_calculations/matrix_calculator.h"
#include "../complex_modules/realtime_analytics.h"
#include "../complex_modules/distributed_computing.h"
#include "ai_optimization.h"
#include <stdint.h>
#include <stdbool.h>

// Magic numbers pour protection
#define AI_CONFIG_MANAGER_MAGIC 0xC0FF1C91
#define AI_CONFIG_DESTROYED_MAGIC 0xDEAD91C0

// Types de modules gérés dynamiquement
typedef enum {
    MODULE_PARETO_OPTIMIZER,
    MODULE_SIMD_OPTIMIZER,
    MODULE_ZERO_COPY_ALLOCATOR,
    MODULE_PARALLEL_PROCESSOR,
    MODULE_NEURAL_NETWORK,
    MODULE_QUANTUM_SIMULATOR,
    // MODULE_HOMOMORPHIC_ENCRYPTION, // REMOVED - No homomorphic functionality
    MODULE_MATRIX_CALCULATOR,
    MODULE_REALTIME_ANALYTICS,
    MODULE_DISTRIBUTED_COMPUTING,
    MODULE_AI_OPTIMIZATION,
    MODULE_COUNT
} ai_managed_module_type_e;

// Structure unifiée de configuration dynamique
typedef struct {
    ai_managed_module_type_e module_type;
    void* config_data;
    size_t config_size;
    bool is_active;
    double performance_weight;
    uint64_t last_updated_timestamp;
    char module_name[64];
    void* memory_address;
} ai_module_config_t;

// Gestionnaire centralisé IA pour tous paramètres système
typedef struct {
    ai_module_config_t* managed_configs[MODULE_COUNT];
    size_t active_modules_count;
    bool auto_optimization_enabled;
    double global_performance_target;
    uint64_t optimization_cycles_count;
    double learning_rate_global;
    ai_agent_t* master_agent;
    lum_group_t* system_state_lums;
    void* memory_address;
    uint32_t manager_magic;
} ai_dynamic_config_manager_t;

// Résultat d'optimisation dynamique système
typedef struct {
    bool optimization_success;
    double performance_improvement_ratio;
    size_t modules_optimized_count;
    double total_optimization_time_ms;
    char optimization_strategy[128];
    lum_group_t* optimized_parameters;
    void* memory_address;
} ai_dynamic_optimization_result_t;

// === FONCTIONS PRINCIPALES GESTION DYNAMIQUE ===

// Création gestionnaire IA centralisé
ai_dynamic_config_manager_t* ai_dynamic_config_manager_create(void);

// Destruction gestionnaire
void ai_dynamic_config_manager_destroy(ai_dynamic_config_manager_t** manager_ptr);

// Enregistrement module pour gestion dynamique IA
bool ai_register_module_for_management(ai_dynamic_config_manager_t* manager,
                                       ai_managed_module_type_e module_type,
                                       void* config_data,
                                       size_t config_size);

// Optimisation automatique TOUS paramètres système
ai_dynamic_optimization_result_t* ai_optimize_all_system_parameters(
    ai_dynamic_config_manager_t* manager,
    double performance_target);

// Mise à jour dynamique paramètre spécifique
bool ai_update_module_parameter(ai_dynamic_config_manager_t* manager,
                               ai_managed_module_type_e module_type,
                               const char* parameter_name,
                               void* new_value,
                               size_t value_size);

// Apprentissage continu performance système
bool ai_learn_from_system_performance(ai_dynamic_config_manager_t* manager,
                                     double current_performance,
                                     double target_performance);

// === FONCTIONS GESTION SPÉCIALISÉE PAR MODULE ===

// Configuration dynamique Pareto Optimizer
bool ai_configure_pareto_optimizer(ai_dynamic_config_manager_t* manager,
                                  bool enable_simd,
                                  bool enable_memory_pooling,
                                  bool enable_parallel,
                                  bool enable_crypto,
                                  double efficiency_threshold);

// Configuration dynamique SIMD Optimizer
bool ai_configure_simd_optimizer(ai_dynamic_config_manager_t* manager,
                                bool enable_avx512,
                                bool enable_avx2,
                                bool enable_sse42,
                                size_t vector_width);

// Configuration dynamique Neural Network
bool ai_configure_neural_network(ai_dynamic_config_manager_t* manager,
                                 size_t max_epochs,
                                 double convergence_threshold,
                                 bool use_momentum,
                                 double momentum_coefficient,
                                 bool use_dropout,
                                 double dropout_rate);

// Configuration dynamique Quantum Simulator
bool ai_configure_quantum_simulator(ai_dynamic_config_manager_t* manager,
                                   size_t max_qubits,
                                   double decoherence_time,
                                   double gate_error_rate,
                                   bool enable_error_correction);

// Configuration dynamique Homomorphic Encryption - REMOVED
// bool ai_configure_homomorphic_encryption(...); // No homomorphic functionality

// Configuration dynamique Parallel Processor
bool ai_configure_parallel_processor(ai_dynamic_config_manager_t* manager,
                                    size_t worker_count,
                                    bool enable_work_stealing,
                                    bool enable_load_balancing);

// Configuration dynamique Realtime Analytics
bool ai_configure_realtime_analytics(ai_dynamic_config_manager_t* manager,
                                    size_t buffer_size,
                                    double sampling_rate,
                                    bool enable_anomaly_detection,
                                    double anomaly_threshold);

// Configuration dynamique Distributed Computing
bool ai_configure_distributed_computing(ai_dynamic_config_manager_t* manager,
                                       size_t max_nodes,
                                       double heartbeat_interval,
                                       bool enable_fault_tolerance,
                                       double load_balancing_threshold);

// === FONCTIONS MONITORING ET ANALYTICS ===

// Collecte métriques performance TOUS modules
bool ai_collect_all_modules_metrics(ai_dynamic_config_manager_t* manager,
                                   double* performance_metrics,
                                   size_t metrics_count);

// Prédiction optimale paramètres basée apprentissage
ai_dynamic_optimization_result_t* ai_predict_optimal_parameters(
    ai_dynamic_config_manager_t* manager,
    lum_group_t* current_system_state);

// Export configuration système complète
bool ai_export_system_configuration(ai_dynamic_config_manager_t* manager,
                                   const char* filename);

// Import et application configuration système
bool ai_import_system_configuration(ai_dynamic_config_manager_t* manager,
                                   const char* filename);

// === FONCTIONS STRESS TEST 100M+ ===

// Test stress gestion dynamique 100M+ paramètres
bool ai_stress_test_100m_parameters(ai_dynamic_config_manager_t* manager);

// Test optimisation temps réel sous charge extrême
bool ai_stress_test_realtime_optimization(ai_dynamic_config_manager_t* manager,
                                         size_t load_multiplier);

#endif // AI_DYNAMIC_CONFIG_MANAGER_H