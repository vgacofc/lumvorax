#ifndef AI_OPTIMIZATION_H
#define AI_OPTIMIZATION_H

#include "../lum/lum_core.h"
#include "../advanced_calculations/neural_network_processor.h"
#include <stdint.h>
#include <stdbool.h>

// Module Optimisation IA pour LUM/VORAX
// Conforme prompt.txt - nouveau module complexe

// Agent d'optimisation IA
typedef struct {
    neural_network_t* brain;      // Réseau de neurones principal
    lum_group_t* knowledge_base;  // Base de connaissances LUM
    double learning_rate;         // Taux d'apprentissage adaptatif
    uint64_t decisions_made;      // Nombre de décisions prises
    double success_rate;          // Taux de succès [0,1]
    uint64_t experience_count;    // Expériences accumulées
    bool is_learning_enabled;     // Apprentissage actif
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint32_t agent_magic;         // Validation intégrité
} ai_agent_t;

// Environnement d'optimisation
typedef struct {
    lum_group_t* current_state;   // État actuel système
    lum_group_t* target_state;    // État cible optimal
    double* reward_function;      // Fonction de récompense
    size_t state_dimension;       // Dimension espace d'état
    double* action_space;         // Espace d'actions possibles
    size_t action_dimension;      // Dimension espace d'action
    uint64_t episode_count;       // Nombre d'épisodes
    void* memory_address;         // Protection double-free OBLIGATOIRE
    bool environment_initialized; // Environnement initialisé
} optimization_environment_t;

// Algorithme d'apprentissage par renforcement
typedef struct {
    double** q_table;             // Table Q pour Q-learning
    size_t state_size;            // Taille espace d'état
    size_t action_size;           // Taille espace d'action
    double epsilon;               // Facteur exploration [0,1]
    double gamma;                 // Facteur d'actualisation [0,1]
    double alpha;                 // Taux d'apprentissage [0,1]
    uint64_t training_episodes;   // Épisodes d'entraînement
    void* memory_address;         // Protection double-free OBLIGATOIRE
    bool convergence_reached;     // Convergence atteinte
} reinforcement_learner_t;

// Optimiseur génétique pour LUM
typedef struct {
    lum_group_t** population;     // Population de solutions
    size_t population_size;       // Taille population
    double* fitness_scores;       // Scores de fitness
    double mutation_rate;         // Taux de mutation [0,1]
    double crossover_rate;        // Taux de croisement [0,1]
    uint64_t generation_count;    // Nombre de générations
    lum_group_t* best_solution;   // Meilleure solution trouvée
    void* memory_address;         // Protection double-free OBLIGATOIRE
    bool elitism_enabled;         // Élitisme activé
} genetic_optimizer_t;

// Métaheuristique d'optimisation
typedef enum {
    META_GENETIC_ALGORITHM = 0,   // Algorithme génétique
    META_PARTICLE_SWARM = 1,      // Essaim particulaire
    META_SIMULATED_ANNEALING = 2, // Recuit simulé
    META_TABU_SEARCH = 3,         // Recherche tabu
    META_ANT_COLONY = 4,          // Colonie de fourmis
    META_DIFFERENTIAL_EVOLUTION = 5, // Évolution différentielle
    META_HARMONY_SEARCH = 6,      // Recherche harmonique
    META_FIREFLY_ALGORITHM = 7    // Algorithme luciole
} metaheuristic_type_e;

// Configuration optimisation IA
typedef struct {
    metaheuristic_type_e algorithm; // Algorithme sélectionné
    size_t max_iterations;        // Itérations maximales
    double convergence_threshold; // Seuil de convergence
    bool use_parallel_processing; // Traitement parallèle
    size_t thread_count;          // Nombre de threads
    bool enable_adaptive_params;  // Paramètres adaptatifs
    double memory_limit_gb;       // Limite mémoire (GB)
    void* memory_address;         // Protection double-free OBLIGATOIRE
} ai_optimization_config_t;

// Résultat optimisation IA
typedef struct {
    lum_group_t* optimal_solution; // Solution optimale trouvée
    double fitness_score;         // Score de fitness final
    uint64_t iterations_performed;// Itérations effectuées
    uint64_t function_evaluations;// Évaluations fonction objectif
    double convergence_rate;      // Taux de convergence
    bool optimization_success;    // Succès optimisation
    char algorithm_used[64];      // Algorithme utilisé
    uint64_t total_time_ns;       // Temps total (ns)
    void* memory_address;         // Protection double-free OBLIGATOIRE
} ai_optimization_result_t;

// Fonctions principales
ai_agent_t* ai_agent_create(size_t brain_layers[], size_t layer_count);
void ai_agent_destroy(ai_agent_t** agent_ptr);
bool ai_agent_learn_from_experience(ai_agent_t* agent, lum_group_t* state, lum_group_t* action, double reward);
lum_group_t* ai_agent_make_decision(ai_agent_t* agent, lum_group_t* current_state);

optimization_environment_t* optimization_environment_create(size_t state_dim, size_t action_dim);
void optimization_environment_destroy(optimization_environment_t** env_ptr);
double optimization_evaluate_fitness(lum_group_t* solution, optimization_environment_t* env);
bool optimization_step_environment(optimization_environment_t* env, lum_group_t* action);

reinforcement_learner_t* reinforcement_learner_create(size_t state_size, size_t action_size);
void reinforcement_learner_destroy(reinforcement_learner_t** learner_ptr);
bool reinforcement_update_q_table(reinforcement_learner_t* learner, size_t state, size_t action, double reward, size_t next_state);
size_t reinforcement_select_action(reinforcement_learner_t* learner, size_t current_state);

genetic_optimizer_t* genetic_optimizer_create(size_t population_size);
void genetic_optimizer_destroy(genetic_optimizer_t** optimizer_ptr);
bool genetic_evolve_population(genetic_optimizer_t* optimizer, optimization_environment_t* env);
lum_group_t* genetic_crossover(lum_group_t* parent1, lum_group_t* parent2);
bool genetic_mutate(lum_group_t* individual, double mutation_rate);

// Algorithmes d'optimisation avancés
ai_optimization_result_t* ai_optimize_genetic_algorithm(lum_group_t* initial_solution, optimization_environment_t* env, ai_optimization_config_t* config);
ai_optimization_result_t* ai_optimize_particle_swarm(lum_group_t* initial_solution, optimization_environment_t* env, ai_optimization_config_t* config);
ai_optimization_result_t* ai_optimize_simulated_annealing(lum_group_t* initial_solution, optimization_environment_t* env, ai_optimization_config_t* config);
ai_optimization_result_t* ai_optimize_reinforcement_learning(ai_agent_t* agent, optimization_environment_t* env, ai_optimization_config_t* config);

// Multi-objectif et Pareto
ai_optimization_result_t* ai_optimize_multi_objective(lum_group_t* initial_solution, optimization_environment_t* env, double* objective_weights, size_t objective_count, ai_optimization_config_t* config);
bool ai_pareto_dominance_check(lum_group_t* solution1, lum_group_t* solution2, optimization_environment_t* env);
lum_group_t** ai_generate_pareto_front(lum_group_t** solutions, size_t solution_count, optimization_environment_t* env);

// Apprentissage et adaptation
bool ai_train_neural_optimizer(ai_agent_t* agent, lum_group_t** training_data, size_t data_count, ai_optimization_config_t* config);
bool ai_update_learning_parameters(ai_agent_t* agent, double performance_metric);
bool ai_save_learned_model(ai_agent_t* agent, const char* filename);
bool ai_load_learned_model(ai_agent_t* agent, const char* filename);

// Tests stress 100M+ LUMs
bool ai_stress_test_100m_lums(ai_optimization_config_t* config);

// Structure traçage étape de décision
typedef struct {
    uint64_t step_number;
    char operation_name[128];
    double input_values[32];
    size_t input_count;
    double intermediate_result;
    double final_output;
    char calculation_formula[256];
    uint64_t computation_time_ns;
    void* memory_address;
    uint32_t step_magic;
} decision_step_trace_t;

// Fonctions traçage IA complet
bool ai_agent_trace_decision_step(ai_agent_t* agent, decision_step_trace_t* step_trace);
bool ai_agent_save_reasoning_state(ai_agent_t* agent, const char* filename);
bool ai_agent_load_reasoning_state(ai_agent_t* agent, const char* filename);

// Configuration par défaut
ai_optimization_config_t* ai_optimization_config_create_default(void);
void ai_optimization_config_destroy(ai_optimization_config_t** config_ptr);
void ai_optimization_result_destroy(ai_optimization_result_t** result_ptr);
bool ai_validate_solution(lum_group_t* solution, optimization_environment_t* env);
double ai_calculate_diversity(lum_group_t** population, size_t population_size);

// Constantes
#define AI_MAX_POPULATION_SIZE 10000
#define AI_MIN_LEARNING_RATE 1e-6
#define AI_MAX_LEARNING_RATE 1.0
#define AI_DEFAULT_EPSILON 0.1
#define AI_DEFAULT_GAMMA 0.95
#define AI_DEFAULT_ALPHA 0.1
#define AI_MAGIC_NUMBER 0xABCDEF03
#define AI_DESTROYED_MAGIC 0xDEADBEEF

#endif // AI_OPTIMIZATION_H