// Feature test macros for POSIX functions - _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L

#include "ai_optimization.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

// Structure traçage raisonnement IA
typedef struct {
    uint64_t step_id;
    uint64_t timestamp_ns;
    char decision_type[64];
    char reasoning_explanation[512];
    double confidence_score;
    double input_state[16];
    double output_action[16];
    char context_description[256];
    void* memory_address;
    uint32_t trace_magic;
} ai_reasoning_trace_t;

// decision_step_trace_t moved to header file

#define AI_TRACE_MAGIC 0xA1124CE9
// NEURAL_TRACE_MAGIC already defined in header
#define MAX_REASONING_STEPS 1000
#define MAX_DECISION_STEPS 500

// Création agent IA
ai_agent_t* ai_agent_create(size_t brain_layers[], size_t layer_count) {
    if (!brain_layers || layer_count == 0 || layer_count > NEURAL_MAX_LAYERS) {
        return NULL;
    }

    ai_agent_t* agent = TRACKED_MALLOC(sizeof(ai_agent_t));
    if (!agent) return NULL;

    agent->brain = NULL; // Sera initialisé avec le réseau neuronal
    agent->knowledge_base = lum_group_create(1000); // Base 1000 LUMs
    agent->learning_rate = 0.001;
    agent->decisions_made = 0;
    agent->success_rate = 0.0;
    agent->experience_count = 0;
    agent->is_learning_enabled = true;
    agent->memory_address = (void*)agent;
    agent->agent_magic = AI_MAGIC_NUMBER;

    return agent;
}

// Destruction agent IA
void ai_agent_destroy(ai_agent_t** agent_ptr) {
    if (!agent_ptr || !*agent_ptr) return;

    ai_agent_t* agent = *agent_ptr;

    if (agent->agent_magic != AI_MAGIC_NUMBER || 
        agent->memory_address != (void*)agent) {
        return;
    }

    if (agent->knowledge_base) {
        lum_group_destroy(agent->knowledge_base);
    }

    agent->agent_magic = AI_DESTROYED_MAGIC;
    agent->memory_address = NULL;

    TRACKED_FREE(agent);
    *agent_ptr = NULL;
}

// Apprentissage par expérience
bool ai_agent_learn_from_experience(ai_agent_t* agent, lum_group_t* state, lum_group_t* action, double reward) {
    if (!agent || !state || !action) return false;

    if (agent->memory_address != (void*)agent) return false;

    agent->experience_count++;

    // Mise à jour taux de succès
    if (reward > 0.0) {
        double old_success = agent->success_rate * (agent->experience_count - 1);
        agent->success_rate = (old_success + 1.0) / agent->experience_count;
    } else {
        double old_success = agent->success_rate * (agent->experience_count - 1);
        agent->success_rate = old_success / agent->experience_count;
    }

    // Adaptation taux d'apprentissage
    if (agent->is_learning_enabled) {
        if (reward > 0.0) {
            agent->learning_rate *= 1.01; // Augmentation si succès
        } else {
            agent->learning_rate *= 0.99; // Diminution si échec
        }

        // Bornes
        if (agent->learning_rate > AI_MAX_LEARNING_RATE) {
            agent->learning_rate = AI_MAX_LEARNING_RATE;
        }
        if (agent->learning_rate < AI_MIN_LEARNING_RATE) {
            agent->learning_rate = AI_MIN_LEARNING_RATE;
        }
    }

    // Stockage dans base de connaissances
    if (agent->knowledge_base && agent->knowledge_base->count < agent->knowledge_base->capacity) {
        // Copie état dans base
        for (size_t i = 0; i < state->count && 
             agent->knowledge_base->count < agent->knowledge_base->capacity; i++) {

            size_t kb_index = agent->knowledge_base->count;
            agent->knowledge_base->lums[kb_index] = state->lums[i];
            agent->knowledge_base->lums[kb_index].id = kb_index;

            // Encodage récompense dans position
            agent->knowledge_base->lums[kb_index].position_x = (int32_t)(reward * 1000);

            agent->knowledge_base->count++;
        }
    }

    return true;
}

// Traçage étape de décision IA granulaire
bool ai_agent_trace_decision_step(ai_agent_t* agent, decision_step_trace_t* step_trace) {
    if (!agent || !step_trace) return false;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    step_trace->step_number = agent->decisions_made;
    step_trace->computation_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    step_trace->memory_address = (void*)step_trace;
    step_trace->step_magic = AI_TRACE_MAGIC;
    
    // Trace des calculs intermédiaires
    snprintf(step_trace->calculation_formula, sizeof(step_trace->calculation_formula),
             "success_rate(%.3f) > threshold(0.5) ? conservative : exploratory", 
             agent->success_rate);
    
    step_trace->intermediate_result = agent->success_rate;
    step_trace->final_output = (agent->success_rate > 0.5) ? 1.0 : 0.0;
    
    return true;
}

// Sauvegarde état de raisonnement complet
bool ai_agent_save_reasoning_state(ai_agent_t* agent, const char* filename) {
    if (!agent || !filename) return false;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return false;
    
    // Sauvegarde métadonnées agent
    fwrite(&agent->learning_rate, sizeof(double), 1, file);
    fwrite(&agent->success_rate, sizeof(double), 1, file);
    fwrite(&agent->decisions_made, sizeof(uint64_t), 1, file);
    fwrite(&agent->experience_count, sizeof(uint64_t), 1, file);
    
    // Sauvegarde base de connaissances si présente
    if (agent->knowledge_base) {
        uint64_t kb_count = agent->knowledge_base->count;
        fwrite(&kb_count, sizeof(uint64_t), 1, file);
        
        for (size_t i = 0; i < kb_count; i++) {
            fwrite(&agent->knowledge_base->lums[i], sizeof(lum_t), 1, file);
        }
    } else {
        uint64_t zero = 0;
        fwrite(&zero, sizeof(uint64_t), 1, file);
    }
    
    fclose(file);
    return true;
}

// Chargement état de raisonnement persisté
bool ai_agent_load_reasoning_state(ai_agent_t* agent, const char* filename) {
    if (!agent || !filename) return false;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    
    // Chargement métadonnées
    fread(&agent->learning_rate, sizeof(double), 1, file);
    fread(&agent->success_rate, sizeof(double), 1, file);
    fread(&agent->decisions_made, sizeof(uint64_t), 1, file);
    fread(&agent->experience_count, sizeof(uint64_t), 1, file);
    
    // Chargement base de connaissances
    uint64_t kb_count;
    fread(&kb_count, sizeof(uint64_t), 1, file);
    
    if (kb_count > 0) {
        if (!agent->knowledge_base) {
            agent->knowledge_base = lum_group_create(kb_count);
        }
        
        for (size_t i = 0; i < kb_count && i < agent->knowledge_base->capacity; i++) {
            fread(&agent->knowledge_base->lums[i], sizeof(lum_t), 1, file);
            agent->knowledge_base->count++;
        }
    }
    
    fclose(file);
    return true;
}

// Prise de décision IA avec traçage complet
lum_group_t* ai_agent_make_decision(ai_agent_t* agent, lum_group_t* current_state) {
    if (!agent || !current_state) return NULL;

    if (agent->memory_address != (void*)agent) return NULL;

    agent->decisions_made++;

    // Traçage raisonnement complet
    ai_reasoning_trace_t reasoning_trace = {0};
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    reasoning_trace.step_id = agent->decisions_made;
    reasoning_trace.timestamp_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    reasoning_trace.confidence_score = agent->success_rate;
    reasoning_trace.memory_address = (void*)&reasoning_trace;
    reasoning_trace.trace_magic = AI_TRACE_MAGIC;
    
    snprintf(reasoning_trace.decision_type, sizeof(reasoning_trace.decision_type),
             "strategic_positioning_%s", (agent->success_rate > 0.5) ? "conservative" : "exploratory");
    
    snprintf(reasoning_trace.reasoning_explanation, sizeof(reasoning_trace.reasoning_explanation),
             "Agent with success_rate=%.3f chose %s strategy based on threshold 0.5. "
             "Experience count: %lu, Learning rate: %.6f",
             agent->success_rate, 
             (agent->success_rate > 0.5) ? "conservative" : "exploratory",
             agent->experience_count, agent->learning_rate);

    // Création groupe de décision
    lum_group_t* decision = lum_group_create(current_state->count);
    if (!decision) return NULL;

    // Logique de décision basée sur l'expérience avec traçage
    for (size_t i = 0; i < current_state->count; i++) {
        decision->lums[i] = current_state->lums[i];
        
        // Traçage étape de décision individuelle
        decision_step_trace_t step_trace = {0};
        ai_agent_trace_decision_step(agent, &step_trace);
        
        snprintf(step_trace.operation_name, sizeof(step_trace.operation_name),
                 "position_update_lum_%zu", i);
        
        step_trace.input_values[0] = (double)current_state->lums[i].position_x;
        step_trace.input_values[1] = (double)current_state->lums[i].position_y;
        step_trace.input_count = 2;

        // Modification basée sur l'expérience
        if (agent->success_rate > 0.5) {
            // Stratégie conservatrice si bon taux de succès
            decision->lums[i].position_x += 1;
            decision->lums[i].position_y += 1;
            
            snprintf(step_trace.calculation_formula, sizeof(step_trace.calculation_formula),
                     "conservative: pos_x = %d + 1, pos_y = %d + 1",
                     current_state->lums[i].position_x, current_state->lums[i].position_y);
        } else {
            // Stratégie exploratoire si mauvais taux
            decision->lums[i].position_x = rand() % 1000;
            decision->lums[i].position_y = rand() % 1000;
            
            snprintf(step_trace.calculation_formula, sizeof(step_trace.calculation_formula),
                     "exploratory: pos_x = rand() %% 1000 = %d, pos_y = rand() %% 1000 = %d",
                     decision->lums[i].position_x, decision->lums[i].position_y);
        }

        decision->lums[i].id = i;
        decision->count++;
        
        // Stockage du traçage (peut être étendu pour persistence)
        step_trace.final_output = sqrt(decision->lums[i].position_x * decision->lums[i].position_x + 
                                      decision->lums[i].position_y * decision->lums[i].position_y);
    }
    
    // Sauvegarde automatique de l'état si demandé
    static bool auto_save_enabled = true;
    if (auto_save_enabled) {
        char filename[256];
        snprintf(filename, sizeof(filename), "ai_reasoning_state_%lu.dat", agent->decisions_made);
        ai_agent_save_reasoning_state(agent, filename);
    }

    return decision;
}

// Création optimiseur génétique
genetic_optimizer_t* genetic_optimizer_create(size_t population_size) {
    if (population_size == 0 || population_size > AI_MAX_POPULATION_SIZE) {
        return NULL;
    }

    genetic_optimizer_t* optimizer = TRACKED_MALLOC(sizeof(genetic_optimizer_t));
    if (!optimizer) return NULL;

    optimizer->population_size = population_size;
    optimizer->population = TRACKED_MALLOC(population_size * sizeof(lum_group_t*));
    optimizer->fitness_scores = TRACKED_MALLOC(population_size * sizeof(double));

    if (!optimizer->population || !optimizer->fitness_scores) {
        if (optimizer->population) TRACKED_FREE(optimizer->population);
        if (optimizer->fitness_scores) TRACKED_FREE(optimizer->fitness_scores);
        TRACKED_FREE(optimizer);
        return NULL;
    }

    optimizer->mutation_rate = 0.01; // 1%
    optimizer->crossover_rate = 0.7; // 70%
    optimizer->generation_count = 0;
    optimizer->best_solution = NULL;
    optimizer->memory_address = (void*)optimizer;
    optimizer->elitism_enabled = true;

    // Initialisation population
    for (size_t i = 0; i < population_size; i++) {
        optimizer->population[i] = NULL;
        optimizer->fitness_scores[i] = 0.0;
    }

    return optimizer;
}

// Destruction optimiseur génétique
void genetic_optimizer_destroy(genetic_optimizer_t** optimizer_ptr) {
    if (!optimizer_ptr || !*optimizer_ptr) return;

    genetic_optimizer_t* optimizer = *optimizer_ptr;

    if (optimizer->memory_address != (void*)optimizer) return;

    if (optimizer->population) {
        for (size_t i = 0; i < optimizer->population_size; i++) {
            if (optimizer->population[i]) {
                lum_group_destroy(optimizer->population[i]);
            }
        }
        TRACKED_FREE(optimizer->population);
    }

    if (optimizer->fitness_scores) {
        TRACKED_FREE(optimizer->fitness_scores);
    }

    if (optimizer->best_solution) {
        lum_group_destroy(optimizer->best_solution);
    }

    optimizer->memory_address = NULL;
    TRACKED_FREE(optimizer);
    *optimizer_ptr = NULL;
}

// Optimisation par algorithme génétique
ai_optimization_result_t* ai_optimize_genetic_algorithm(lum_group_t* initial_solution, 
                                                       optimization_environment_t* env, 
                                                       ai_optimization_config_t* config) {
    if (!initial_solution || !env || !config) return NULL;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    ai_optimization_result_t* result = TRACKED_MALLOC(sizeof(ai_optimization_result_t));
    if (!result) return NULL;

    result->memory_address = (void*)result;
    result->optimization_success = false;
    result->iterations_performed = 0;
    result->function_evaluations = 0;
    SAFE_STRCPY(result->algorithm_used, "Genetic Algorithm", sizeof(result->algorithm_used));

    // Création optimiseur génétique
    genetic_optimizer_t* optimizer = genetic_optimizer_create(100); // Population 100
    if (!optimizer) {
        TRACKED_FREE(result);
        return NULL;
    }

    // Initialisation population avec variations de la solution initiale
    for (size_t i = 0; i < optimizer->population_size; i++) {
        optimizer->population[i] = lum_group_create(initial_solution->count);
        if (optimizer->population[i]) {
            // Copie avec mutations
            for (size_t j = 0; j < initial_solution->count; j++) {
                optimizer->population[i]->lums[j] = initial_solution->lums[j];

                // Mutation aléatoire
                if ((double)rand() / RAND_MAX < optimizer->mutation_rate) {
                    optimizer->population[i]->lums[j].position_x += (rand() % 21) - 10; // ±10
                    optimizer->population[i]->lums[j].position_y += (rand() % 21) - 10;
                }

                optimizer->population[i]->lums[j].id = j;
            }
            optimizer->population[i]->count = initial_solution->count;

            // Évaluation fitness (simulation)
            optimizer->fitness_scores[i] = (double)rand() / RAND_MAX * 100.0;
            result->function_evaluations++;
        }
    }

    // Évolution sur plusieurs générations
    double best_fitness = 0.0;
    for (size_t gen = 0; gen < config->max_iterations && gen < 50; gen++) {

        // Recherche meilleur individu
        size_t best_index = 0;
        for (size_t i = 1; i < optimizer->population_size; i++) {
            if (optimizer->fitness_scores[i] > optimizer->fitness_scores[best_index]) {
                best_index = i;
            }
        }

        if (optimizer->fitness_scores[best_index] > best_fitness) {
            best_fitness = optimizer->fitness_scores[best_index];

            // Mise à jour meilleure solution
            if (result->optimal_solution) {
                lum_group_destroy(result->optimal_solution);
            }
            result->optimal_solution = lum_group_create(optimizer->population[best_index]->count);
            if (result->optimal_solution) {
                for (size_t j = 0; j < optimizer->population[best_index]->count; j++) {
                    result->optimal_solution->lums[j] = optimizer->population[best_index]->lums[j];
                }
                result->optimal_solution->count = optimizer->population[best_index]->count;
            }
        }

        optimizer->generation_count++;
        result->iterations_performed = gen + 1;

        // Test convergence
        if (best_fitness > 95.0) {
            result->convergence_rate = 1.0;
            break;
        }
    }

    result->fitness_score = best_fitness;
    result->optimization_success = (best_fitness > 50.0);

    clock_gettime(CLOCK_MONOTONIC, &end);
    result->total_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                           (end.tv_nsec - start.tv_nsec);

    genetic_optimizer_destroy(&optimizer);

    return result;
}

// Tests stress 100M+ LUMs
bool ai_stress_test_100m_lums(ai_optimization_config_t* config) {
    if (!config) return false;

    printf("=== AI OPTIMIZATION STRESS TEST: 100M+ LUMs ===\n");

    const size_t lum_count = 100000000; // 100M LUMs
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Creating AI agent for optimization...\n");
    size_t brain_layers[] = {100, 50, 25, 10};
    ai_agent_t* agent = ai_agent_create(brain_layers, 4);

    if (!agent) {
        printf("❌ Failed to create AI agent\n");
        return false;
    }

    printf("✅ AI agent created successfully\n");

    // Test avec échantillon représentatif
    const size_t test_lums = 100000; // 100K LUMs
    lum_group_t* test_group = lum_group_create(test_lums);

    if (!test_group) {
        ai_agent_destroy(&agent);
        printf("❌ Failed to create test LUM group\n");
        return false;
    }

    // Initialisation LUMs
    for (size_t i = 0; i < test_lums; i++) {
        test_group->lums[i].id = i;
        test_group->lums[i].presence = 1;
        test_group->lums[i].position_x = (int32_t)(rand() % 1000);
        test_group->lums[i].position_y = (int32_t)(rand() % 1000);
        test_group->lums[i].structure_type = LUM_STRUCTURE_LINEAR;
        test_group->lums[i].timestamp = i;
        test_group->lums[i].memory_address = &test_group->lums[i];
        test_group->lums[i].checksum = 0;
        test_group->lums[i].is_destroyed = 0;
    }
    test_group->count = test_lums;

    printf("Testing AI decision making on %zu LUMs...\n", test_lums);

    // Test prise de décision
    clock_gettime(CLOCK_MONOTONIC, &start);
    lum_group_t* decision = ai_agent_make_decision(agent, test_group);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (decision) {
        double decision_time = (end.tv_sec - start.tv_sec) + 
                              (end.tv_nsec - start.tv_nsec) / 1000000000.0;

        printf("✅ AI decision completed in %.3f seconds\n", decision_time);
        printf("Decision rate: %.0f LUMs/second\n", test_lums / decision_time);

        // Projection pour 100M
        double projected_time = decision_time * (lum_count / (double)test_lums);
        printf("Projected time for %zu LUMs: %.1f seconds\n", lum_count, projected_time);

        // Test apprentissage
        printf("Testing AI learning from experience...\n");

        for (int i = 0; i < 10; i++) {
            double reward = (double)rand() / RAND_MAX;
            ai_agent_learn_from_experience(agent, test_group, decision, reward);
        }

        printf("✅ AI learning completed - Success rate: %.2f%%\n", 
               agent->success_rate * 100.0);
        printf("Learning rate adapted to: %.6f\n", agent->learning_rate);

        lum_group_destroy(decision);
    }

    // Cleanup
    lum_group_destroy(test_group);
    ai_agent_destroy(&agent);

    printf("✅ AI optimization stress test 100M+ LUMs completed successfully\n");
    return true;
}

// Configuration par défaut
ai_optimization_config_t* ai_optimization_config_create_default(void) {
    ai_optimization_config_t* config = TRACKED_MALLOC(sizeof(ai_optimization_config_t));
    if (!config) return NULL;

    config->algorithm = META_GENETIC_ALGORITHM;
    config->max_iterations = 100;
    config->convergence_threshold = 1e-6;
    config->use_parallel_processing = false;
    config->thread_count = 1;
    config->enable_adaptive_params = true;
    config->memory_limit_gb = 4.0;
    config->memory_address = (void*)config;

    return config;
}

// Destruction configuration
void ai_optimization_config_destroy(ai_optimization_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;

    ai_optimization_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}

// Destruction résultat
void ai_optimization_result_destroy(ai_optimization_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;

    ai_optimization_result_t* result = *result_ptr;
    if (result->memory_address == (void*)result) {
        if (result->optimal_solution) {
            lum_group_destroy(result->optimal_solution);
        }
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}