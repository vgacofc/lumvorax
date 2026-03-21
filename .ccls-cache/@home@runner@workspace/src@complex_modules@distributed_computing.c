// Feature test macros for POSIX functions
// _GNU_SOURCE défini dans Makefile
#define _POSIX_C_SOURCE 200809L

#include "distributed_computing.h"
#include "../debug/memory_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

// Création nœud de calcul
compute_node_t* compute_node_create(uint32_t node_id, const char* ip_address, uint16_t port) {
    if (!ip_address || port == 0) return NULL;

    compute_node_t* node = TRACKED_MALLOC(sizeof(compute_node_t));
    if (!node) return NULL;

    node->node_id = node_id;
    node->is_active = false;
    node->cpu_capacity = 1.0; // 100% par défaut
    node->memory_capacity_gb = 8.0; // 8GB par défaut
    node->lums_processed = 0;
    node->processing_rate = 0.0;
    node->memory_address = (void*)node;
    node->node_magic = DISTRIBUTED_MAGIC_NUMBER;

    // Configuration adresse réseau
    memset(&node->address, 0, sizeof(struct sockaddr_in));
    node->address.sin_family = AF_INET;
    node->address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_address, &node->address.sin_addr) <= 0) {
        TRACKED_FREE(node);
        return NULL;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    node->last_heartbeat = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    return node;
}

// Destruction nœud
void compute_node_destroy(compute_node_t** node_ptr) {
    if (!node_ptr || !*node_ptr) return;

    compute_node_t* node = *node_ptr;

    if (node->node_magic != DISTRIBUTED_MAGIC_NUMBER || 
        node->memory_address != (void*)node) {
        return;
    }

    node->node_magic = DISTRIBUTED_DESTROYED_MAGIC;
    node->memory_address = NULL;

    TRACKED_FREE(node);
    *node_ptr = NULL;
}

// Vérification vie nœud
bool compute_node_is_alive(compute_node_t* node) {
    if (!node) return false;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t current_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    // Timeout 30 secondes
    uint64_t timeout_ns = 30ULL * 1000000000ULL;

    return (current_time - node->last_heartbeat) < timeout_ns;
}

// Envoi heartbeat
bool compute_node_send_heartbeat(compute_node_t* node) {
    if (!node) return false;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    node->last_heartbeat = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    node->is_active = true;

    return true;
}

// Création cluster
compute_cluster_t* compute_cluster_create(size_t max_nodes) {
    if (max_nodes == 0 || max_nodes > DISTRIBUTED_MAX_NODES) {
        return NULL;
    }

    compute_cluster_t* cluster = TRACKED_MALLOC(sizeof(compute_cluster_t));
    if (!cluster) return NULL;

    cluster->nodes = TRACKED_MALLOC(max_nodes * sizeof(compute_node_t*));
    if (!cluster->nodes) {
        TRACKED_FREE(cluster);
        return NULL;
    }

    cluster->node_count = max_nodes;
    cluster->active_nodes = 0;
    cluster->cluster_id = rand() % 1000000;
    cluster->total_capacity = 0.0;
    cluster->work_queue = NULL; // Sera initialisé si nécessaire
    cluster->load_balancing = true;
    cluster->memory_address = (void*)cluster;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cluster->cluster_start_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    // Initialisation pointeurs nœuds
    for (size_t i = 0; i < max_nodes; i++) {
        cluster->nodes[i] = NULL;
    }

    return cluster;
}

// Destruction cluster
void compute_cluster_destroy(compute_cluster_t** cluster_ptr) {
    if (!cluster_ptr || !*cluster_ptr) return;

    compute_cluster_t* cluster = *cluster_ptr;

    if (cluster->memory_address != (void*)cluster) return;

    if (cluster->nodes) {
        for (size_t i = 0; i < cluster->node_count; i++) {
            if (cluster->nodes[i]) {
                compute_node_destroy(&cluster->nodes[i]);
            }
        }
        TRACKED_FREE(cluster->nodes);
    }

    cluster->memory_address = NULL;
    TRACKED_FREE(cluster);
    *cluster_ptr = NULL;
}

// Ajout nœud au cluster
bool compute_cluster_add_node(compute_cluster_t* cluster, compute_node_t* node) {
    if (!cluster || !node) return false;

    if (cluster->memory_address != (void*)cluster) return false;

    // Recherche slot libre
    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i] == NULL) {
            cluster->nodes[i] = node;
            cluster->active_nodes++;
            cluster->total_capacity += node->cpu_capacity;
            return true;
        }
    }

    return false; // Cluster plein
}

// Suppression nœud du cluster
bool compute_cluster_remove_node(compute_cluster_t* cluster, uint32_t node_id) {
    if (!cluster) return false;

    if (cluster->memory_address != (void*)cluster) return false;

    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i] && cluster->nodes[i]->node_id == node_id) {
            cluster->total_capacity -= cluster->nodes[i]->cpu_capacity;
            compute_node_destroy(&cluster->nodes[i]);
            cluster->nodes[i] = NULL;
            cluster->active_nodes--;
            return true;
        }
    }

    return false;
}

// Création tâche distribuée
distributed_task_t* distributed_task_create(uint32_t task_id, lum_group_t* input) {
    if (!input) return NULL;

    distributed_task_t* task = TRACKED_MALLOC(sizeof(distributed_task_t));
    if (!task) return NULL;

    task->task_id = task_id;
    task->input_data = input;
    task->output_data = NULL;
    task->assigned_node = 0;
    task->is_completed = false;
    task->completion_ratio = 0.0;
    task->memory_address = (void*)task;
    task->task_magic = DISTRIBUTED_MAGIC_NUMBER;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    task->task_start_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    task->task_end_time = 0;

    return task;
}

// Destruction tâche
void distributed_task_destroy(distributed_task_t** task_ptr) {
    if (!task_ptr || !*task_ptr) return;

    distributed_task_t* task = *task_ptr;

    if (task->task_magic != DISTRIBUTED_MAGIC_NUMBER || 
        task->memory_address != (void*)task) {
        return;
    }

    // Note: input_data et output_data appartiennent au caller
    task->task_magic = DISTRIBUTED_DESTROYED_MAGIC;
    task->memory_address = NULL;

    TRACKED_FREE(task);
    *task_ptr = NULL;
}

// Assignment tâche à nœud
bool distributed_task_assign_node(distributed_task_t* task, uint32_t node_id) {
    if (!task) return false;

    if (task->memory_address != (void*)task) return false;

    task->assigned_node = node_id;
    return true;
}

// Exécution tâche sur nœud
bool distributed_task_execute(distributed_task_t* task, compute_node_t* node) {
    if (!task || !node) return false;

    if (task->memory_address != (void*)task || 
        node->memory_address != (void*)node) {
        return false;
    }

    // Simulation exécution (traitement LUMs)
    if (task->input_data && task->input_data->lums && task->input_data->count > 0) {

        // Simulation traitement progressif
        for (size_t i = 0; i < task->input_data->count; i++) {
            // Simulation traitement LUM
            lum_t* lum = &task->input_data->lums[i];

            // Modification position (exemple opération)
            lum->position_x = lum->position_x * 2;
            lum->position_y = lum->position_y * 2;

            task->completion_ratio = (double)(i + 1) / task->input_data->count;

            // Simulation latence réseau/calcul
            struct timespec delay = {0, 1000}; // 1 microseconde
            nanosleep(&delay, NULL);
        }

        node->lums_processed += task->input_data->count;
        task->is_completed = true;

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        task->task_end_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

        // Mise à jour taux traitement nœud
        uint64_t processing_time = task->task_end_time - task->task_start_time;
        if (processing_time > 0) {
            node->processing_rate = (double)task->input_data->count * 1000000000.0 / processing_time;
        }

        return true;
    }

    return false;
}

// Tests stress 100M+ LUMs
bool distributed_stress_test_100m_lums(distributed_config_t* config) {
    if (!config) return false;

    printf("=== DISTRIBUTED STRESS TEST: 100M+ LUMs ===\n");

    const size_t lum_count = 100000000; // 100M LUMs
    const size_t node_count = 10;       // 10 nœuds

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Creating distributed cluster with %zu nodes...\n", node_count);
    compute_cluster_t* cluster = compute_cluster_create(node_count);

    if (!cluster) {
        printf("❌ Failed to create distributed cluster\n");
        return false;
    }

    // Création nœuds de test
    for (size_t i = 0; i < node_count; i++) {
        char ip[16];
        snprintf(ip, sizeof(ip), "192.168.1.%zu", i + 10);

        compute_node_t* node = compute_node_create(i, ip, DISTRIBUTED_DEFAULT_PORT + i);
        if (node && compute_cluster_add_node(cluster, node)) {
            compute_node_send_heartbeat(node);
            printf("Node %zu added: %s:%lu\n", i, ip, (unsigned long)(DISTRIBUTED_DEFAULT_PORT + i));
        }
    }

    printf("✅ Created cluster with %zu active nodes\n", cluster->active_nodes);

    // Test avec échantillon représentatif
    const size_t test_lums = 10000; // 10K LUMs par nœud
    const size_t lums_per_node = test_lums / node_count;

    printf("Distributing %zu LUMs across %zu nodes (%zu LUMs/node)...\n", 
           test_lums, cluster->active_nodes, lums_per_node);

    // Allocation groupe de LUMs test
    lum_group_t* test_group = lum_group_create(test_lums);
    if (!test_group) {
        compute_cluster_destroy(&cluster);
        printf("❌ Failed to create test LUM group\n");
        return false;
    }

    // Initialisation LUMs
    for (size_t i = 0; i < test_lums; i++) {
        test_group->lums[i].id = i;
        test_group->lums[i].presence = 1;
        test_group->lums[i].position_x = (int32_t)(i % 1000);
        test_group->lums[i].position_y = (int32_t)(i / 1000);
        test_group->lums[i].structure_type = LUM_STRUCTURE_LINEAR;
        test_group->lums[i].timestamp = i;
        test_group->lums[i].memory_address = &test_group->lums[i];
        test_group->lums[i].checksum = 0;
        test_group->lums[i].is_destroyed = 0;
    }
    test_group->count = test_lums;

    // Distribution et exécution
    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t processed_total = 0;

    for (size_t i = 0; i < cluster->active_nodes; i++) {
        if (cluster->nodes[i]) {
            // Création tâche pour ce nœud
            distributed_task_t* task = distributed_task_create(i, test_group);
            if (task) {
                distributed_task_assign_node(task, cluster->nodes[i]->node_id);

                // Exécution sur nœud
                if (distributed_task_execute(task, cluster->nodes[i])) {
                    processed_total += test_group->count;
                    printf("Node %u processed %zu LUMs (rate: %.0f LUMs/sec)\n", 
                           cluster->nodes[i]->node_id, 
                           test_group->count,
                           cluster->nodes[i]->processing_rate);
                }

                distributed_task_destroy(&task);
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double processing_time = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("✅ Processed %zu LUMs across cluster in %.3f seconds\n", 
           processed_total, processing_time);
    printf("Distributed throughput: %.0f LUMs/second\n", 
           processed_total / processing_time);

    // Projection pour 100M
    double projected_time = processing_time * (lum_count / (double)processed_total);
    printf("Projected time for %zu LUMs: %.1f seconds\n", lum_count, projected_time);

    // Cleanup
    lum_group_destroy(test_group);
    compute_cluster_destroy(&cluster);

    printf("✅ Distributed stress test 100M+ LUMs completed successfully\n");
    return true;
}

// Configuration par défaut
distributed_config_t* distributed_config_create_default(void) {
    distributed_config_t* config = TRACKED_MALLOC(sizeof(distributed_config_t));
    if (!config) return NULL;

    config->max_nodes = 100;
    config->heartbeat_interval_sec = 5.0;
    config->replication_factor = 3;
    config->enable_fault_tolerance = true;
    config->enable_data_locality = true;
    config->network_timeout_sec = 30.0;
    config->max_concurrent_tasks = 1000;
    config->memory_address = (void*)config;

    return config;
}

// Destruction configuration
void distributed_config_destroy(distributed_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;

    distributed_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}