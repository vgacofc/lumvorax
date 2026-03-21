#ifndef REALTIME_ANALYTICS_H
#define REALTIME_ANALYTICS_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Module Analytique Temps Réel pour LUM/VORAX
// Conforme prompt.txt - nouveau module complexe

// Stream de données LUM en temps réel
typedef struct {
    lum_t** data_buffer;          // Buffer circulaire LUMs
    size_t buffer_size;           // Taille du buffer
    size_t write_index;           // Index d'écriture
    size_t read_index;            // Index de lecture
    uint64_t total_processed;     // Total LUMs traités
    double processing_rate;       // LUMs/seconde
    uint64_t timestamp_start;     // Début du stream
    uint64_t timestamp_last;      // Dernière activité
    void* memory_address;         // Protection double-free OBLIGATOIRE
    bool is_streaming;            // État du streaming
} realtime_stream_t;

// Métriques analytiques en temps réel
typedef struct {
    double mean_position_x;       // Position X moyenne
    double mean_position_y;       // Position Y moyenne
    double variance_x;            // Variance position X
    double variance_y;            // Variance position Y
    double presence_ratio;        // Ratio présence/absence
    uint64_t total_lums;          // Total LUMs analysés
    double throughput_lums_sec;   // Débit LUMs/seconde
    double latency_ns;            // Latence moyenne (ns)
    double jitter_ns;             // Gigue temporelle (ns)
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint64_t last_update;         // Dernière mise à jour
} analytics_metrics_t;

// Détecteur d'anomalies temps réel
typedef struct {
    double* baseline_values;      // Valeurs de référence
    double* thresholds;           // Seuils d'anomalie
    size_t metric_count;          // Nombre de métriques
    uint64_t anomaly_count;       // Nombre d'anomalies détectées
    double sensitivity;           // Sensibilité détection [0,1]
    bool adaptive_thresholds;     // Seuils adaptatifs
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint64_t last_anomaly;        // Timestamp dernière anomalie
} anomaly_detector_t;

// Agrégateur multi-temporel
typedef struct {
    analytics_metrics_t* hourly_metrics;   // Métriques horaires
    analytics_metrics_t* daily_metrics;    // Métriques journalières
    analytics_metrics_t* weekly_metrics;   // Métriques hebdomadaires
    analytics_metrics_t* monthly_metrics;  // Métriques mensuelles
    size_t retention_hours;       // Rétention données (heures)
    uint64_t aggregation_start;   // Début agrégation
    void* memory_address;         // Protection double-free OBLIGATOIRE
    bool continuous_mode;         // Mode continu 24/7
} temporal_aggregator_t;

// Configuration analytique temps réel
typedef struct {
    size_t stream_buffer_size;    // Taille buffer streaming
    double sampling_rate_hz;      // Fréquence échantillonnage (Hz)
    bool enable_anomaly_detection;// Détection d'anomalies
    bool enable_temporal_aggregation; // Agrégation temporelle
    size_t max_concurrent_streams;// Streams simultanés max
    double memory_usage_limit_gb; // Limite mémoire (GB)
    bool enable_gpu_acceleration; // Accélération GPU
    void* memory_address;         // Protection double-free OBLIGATOIRE
} analytics_config_t;

// Résultat analyse temps réel
typedef struct {
    analytics_metrics_t* current_metrics; // Métriques actuelles
    anomaly_detector_t* anomalies;        // Anomalies détectées
    temporal_aggregator_t* aggregation;   // Données agrégées
    bool analysis_success;                // Succès analyse
    char error_message[256];              // Message d'erreur
    uint64_t processing_time_ns;          // Temps traitement (ns)
    double cpu_usage_percent;             // Usage CPU (%)
    double memory_usage_gb;               // Usage mémoire (GB)
    void* memory_address;                 // Protection double-free OBLIGATOIRE
} analytics_result_t;

// Fonctions principales
realtime_stream_t* realtime_stream_create(size_t buffer_size);
void realtime_stream_destroy(realtime_stream_t** stream_ptr);
bool realtime_stream_push_lum(realtime_stream_t* stream, lum_t* lum);
lum_t* realtime_stream_pop_lum(realtime_stream_t* stream);

analytics_metrics_t* analytics_metrics_create(void);
void analytics_metrics_destroy(analytics_metrics_t** metrics_ptr);
bool analytics_update_metrics(analytics_metrics_t* metrics, lum_t* lum);
bool analytics_calculate_aggregates(analytics_metrics_t* metrics);

anomaly_detector_t* anomaly_detector_create(size_t metric_count, double sensitivity);
void anomaly_detector_destroy(anomaly_detector_t** detector_ptr);
bool anomaly_detect(anomaly_detector_t* detector, analytics_metrics_t* metrics);
bool anomaly_update_baselines(anomaly_detector_t* detector, analytics_metrics_t* metrics);

temporal_aggregator_t* temporal_aggregator_create(size_t retention_hours);
void temporal_aggregator_destroy(temporal_aggregator_t** aggregator_ptr);
bool temporal_aggregate_metrics(temporal_aggregator_t* aggregator, analytics_metrics_t* current);

// Analyse temps réel avancée
analytics_result_t* realtime_analyze_stream(realtime_stream_t* stream, analytics_config_t* config);
analytics_result_t* realtime_analyze_lum_group(lum_group_t* group, analytics_config_t* config);
bool realtime_start_continuous_analysis(realtime_stream_t* stream, analytics_config_t* config);
bool realtime_stop_continuous_analysis(realtime_stream_t* stream);

// Métriques de performance avancées
double realtime_calculate_throughput(realtime_stream_t* stream);
double realtime_calculate_latency(realtime_stream_t* stream);
double realtime_calculate_jitter(realtime_stream_t* stream);
bool realtime_performance_benchmark(analytics_config_t* config);

// Tests stress 100M+ LUMs
bool analytics_stress_test_100m_lums(analytics_config_t* config);

// Fonction traçage analytique complet
bool realtime_analytics_full_trace(realtime_stream_t* stream, const char* trace_file);

// Configuration par défaut
analytics_config_t* analytics_config_create_default(void);
void analytics_config_destroy(analytics_config_t** config_ptr);
void analytics_result_destroy(analytics_result_t** result_ptr);
bool analytics_validate_stream(realtime_stream_t* stream);
double analytics_calculate_correlation(double* data_x, double* data_y, size_t size);

// Constantes
#define ANALYTICS_MAX_STREAMS 1000
#define ANALYTICS_MIN_BUFFER_SIZE 1000
#define ANALYTICS_MAX_RETENTION_HOURS 8760  // 1 an
#define ANALYTICS_DEFAULT_SAMPLING_RATE 1000.0  // 1 kHz
#define ANALYTICS_MAGIC_NUMBER 0xABCDEF01
#define ANALYTICS_DESTROYED_MAGIC 0xDEADBEEF

#endif // REALTIME_ANALYTICS_H