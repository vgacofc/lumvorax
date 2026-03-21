#ifndef DISTRIBUTED_COMPUTING_H
#define DISTRIBUTED_COMPUTING_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>

// Module Calcul Distribué pour LUM/VORAX
// Conforme prompt.txt - nouveau module complexe

// Nœud de calcul distribué
typedef struct {
    uint32_t node_id;             // Identifiant unique nœud
    struct sockaddr_in address;   // Adresse réseau nœud
    bool is_active;               // État activité nœud
    double cpu_capacity;          // Capacité CPU relative [0,1]
    double memory_capacity_gb;    // Capacité mémoire (GB)
    uint64_t lums_processed;      // Total LUMs traités
    double processing_rate;       // LUMs/seconde
    uint64_t last_heartbeat;      // Dernier battement
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint32_t node_magic;          // Validation intégrité
} compute_node_t;

// Cluster de calcul distribué
typedef struct {
    compute_node_t** nodes;       // Array de nœuds
    size_t node_count;            // Nombre de nœuds
    size_t active_nodes;          // Nœuds actifs
    uint32_t cluster_id;          // Identifiant cluster
    double total_capacity;        // Capacité totale cluster
    lum_group_t* work_queue;      // File d'attente travaux
    bool load_balancing;          // Équilibrage de charge
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint64_t cluster_start_time;  // Démarrage cluster
} compute_cluster_t;

// Tâche de calcul distribué
typedef struct {
    uint32_t task_id;             // Identifiant unique tâche
    lum_group_t* input_data;      // Données d'entrée
    lum_group_t* output_data;     // Données de sortie
    uint32_t assigned_node;       // Nœud assigné
    uint64_t task_start_time;     // Début tâche
    uint64_t task_end_time;       // Fin tâche
    bool is_completed;            // Tâche terminée
    double completion_ratio;      // Ratio completion [0,1]
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint32_t task_magic;          // Validation intégrité
} distributed_task_t;

// Gestionnaire de réplication
typedef struct {
    lum_group_t** replicas;       // Copies de données
    size_t replica_count;         // Nombre de répliques
    uint32_t* replica_nodes;      // Nœuds des répliques
    bool consistency_check;       // Vérification cohérence
    double sync_interval_sec;     // Intervalle synchronisation
    uint64_t last_sync;           // Dernière synchronisation
    void* memory_address;         // Protection double-free OBLIGATOIRE
    bool strong_consistency;      // Cohérence forte
} replication_manager_t;

// Métriques cluster distribué
typedef struct {
    double cluster_utilization;   // Utilisation cluster [0,1]
    double network_latency_ms;    // Latence réseau moyenne (ms)
    double throughput_lums_sec;   // Débit global LUMs/sec
    uint64_t total_tasks;         // Total tâches exécutées
    uint64_t failed_tasks;        // Tâches échouées
    double fault_tolerance;       // Tolérance aux pannes [0,1]
    double data_locality_ratio;   // Ratio localité données [0,1]
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint64_t metrics_timestamp;   // Horodatage métriques
} cluster_metrics_t;

// Configuration calcul distribué
typedef struct {
    size_t max_nodes;             // Nombre maximum de nœuds
    double heartbeat_interval_sec;// Intervalle battement (sec)
    size_t replication_factor;    // Facteur de réplication
    bool enable_fault_tolerance;  // Tolérance aux pannes
    bool enable_data_locality;    // Localité des données
    double network_timeout_sec;   // Timeout réseau (sec)
    size_t max_concurrent_tasks;  // Tâches simultanées max
    void* memory_address;         // Protection double-free OBLIGATOIRE
} distributed_config_t;

// Résultat calcul distribué
typedef struct {
    lum_group_t* final_result;    // Résultat final agrégé
    cluster_metrics_t* metrics;   // Métriques cluster
    distributed_task_t** completed_tasks; // Tâches terminées
    size_t task_count;            // Nombre de tâches
    bool computation_success;     // Succès calcul global
    char error_message[256];      // Message d'erreur
    uint64_t total_time_ns;       // Temps total (ns)
    double speedup_factor;        // Facteur d'accélération
    void* memory_address;         // Protection double-free OBLIGATOIRE
} distributed_result_t;

// Fonctions principales
compute_node_t* compute_node_create(uint32_t node_id, const char* ip_address, uint16_t port);
void compute_node_destroy(compute_node_t** node_ptr);
bool compute_node_is_alive(compute_node_t* node);
bool compute_node_send_heartbeat(compute_node_t* node);

compute_cluster_t* compute_cluster_create(size_t max_nodes);
void compute_cluster_destroy(compute_cluster_t** cluster_ptr);
bool compute_cluster_add_node(compute_cluster_t* cluster, compute_node_t* node);
bool compute_cluster_remove_node(compute_cluster_t* cluster, uint32_t node_id);

distributed_task_t* distributed_task_create(uint32_t task_id, lum_group_t* input);
void distributed_task_destroy(distributed_task_t** task_ptr);
bool distributed_task_assign_node(distributed_task_t* task, uint32_t node_id);
bool distributed_task_execute(distributed_task_t* task, compute_node_t* node);

replication_manager_t* replication_manager_create(size_t replica_count);
void replication_manager_destroy(replication_manager_t** manager_ptr);
bool replication_create_replica(replication_manager_t* manager, lum_group_t* data, uint32_t node_id);
bool replication_sync_replicas(replication_manager_t* manager);

// Algorithmes distribués
distributed_result_t* distributed_map_reduce(compute_cluster_t* cluster, lum_group_t* input_data, distributed_config_t* config);
distributed_result_t* distributed_parallel_vorax(compute_cluster_t* cluster, lum_group_t* input_data, distributed_config_t* config);
bool distributed_load_balance(compute_cluster_t* cluster, distributed_task_t** tasks, size_t task_count);
bool distributed_fault_recovery(compute_cluster_t* cluster, uint32_t failed_node_id);

// Métriques et monitoring
cluster_metrics_t* cluster_metrics_create(void);
void cluster_metrics_destroy(cluster_metrics_t** metrics_ptr);
bool cluster_update_metrics(compute_cluster_t* cluster, cluster_metrics_t* metrics);
double cluster_calculate_efficiency(compute_cluster_t* cluster);

// Communication réseau
bool distributed_send_data(compute_node_t* source, compute_node_t* target, lum_group_t* data);
lum_group_t* distributed_receive_data(compute_node_t* node, uint32_t timeout_ms);
bool distributed_broadcast(compute_cluster_t* cluster, lum_group_t* data);

// Tests stress 100M+ LUMs
bool distributed_stress_test_100m_lums(distributed_config_t* config);
distributed_result_t* distributed_benchmark_scalability(size_t node_count, size_t lum_count, distributed_config_t* config);
distributed_result_t* distributed_test_fault_tolerance(size_t failure_count, distributed_config_t* config);
distributed_result_t* distributed_test_network_latency(double latency_ms, distributed_config_t* config);

// Utilitaires
distributed_config_t* distributed_config_create_default(void);
void distributed_config_destroy(distributed_config_t** config_ptr);
void distributed_result_destroy(distributed_result_t** result_ptr);
bool distributed_validate_cluster(compute_cluster_t* cluster);
double distributed_calculate_network_cost(compute_node_t* source, compute_node_t* target);

// Constantes
#define DISTRIBUTED_MAX_NODES 1000
#define DISTRIBUTED_DEFAULT_PORT 8080
#define DISTRIBUTED_HEARTBEAT_TIMEOUT 30.0  // secondes
#define DISTRIBUTED_MAX_RETRIES 3
#define DISTRIBUTED_MAGIC_NUMBER 0xABCDEF02
#define DISTRIBUTED_DESTROYED_MAGIC 0xDEADBEEF

#endif // DISTRIBUTED_COMPUTING_H