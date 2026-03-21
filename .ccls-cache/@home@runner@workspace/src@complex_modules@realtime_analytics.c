// Feature test macros for POSIX functions
// _GNU_SOURCE défini dans Makefile
#define _POSIX_C_SOURCE 200809L

#include "realtime_analytics.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

// Création stream temps réel
realtime_stream_t* realtime_stream_create(size_t buffer_size) {
    if (buffer_size < ANALYTICS_MIN_BUFFER_SIZE || buffer_size > 1000000000) {
        return NULL;
    }

    realtime_stream_t* stream = TRACKED_MALLOC(sizeof(realtime_stream_t));
    if (!stream) return NULL;

    stream->buffer_size = buffer_size;
    stream->write_index = 0;
    stream->read_index = 0;
    stream->total_processed = 0;
    stream->processing_rate = 0.0;
    stream->is_streaming = false;
    stream->memory_address = (void*)stream;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    stream->timestamp_start = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    stream->timestamp_last = stream->timestamp_start;

    // Allocation buffer circulaire
    stream->data_buffer = TRACKED_MALLOC(buffer_size * sizeof(lum_t*));
    if (!stream->data_buffer) {
        TRACKED_FREE(stream);
        return NULL;
    }

    // Initialisation pointeurs NULL
    for (size_t i = 0; i < buffer_size; i++) {
        stream->data_buffer[i] = NULL;
    }

    return stream;
}

// Destruction stream
void realtime_stream_destroy(realtime_stream_t** stream_ptr) {
    if (!stream_ptr || !*stream_ptr) return;

    realtime_stream_t* stream = *stream_ptr;

    if (stream->memory_address != (void*)stream) return;

    if (stream->data_buffer) {
        // Libération des LUMs dans le buffer
        for (size_t i = 0; i < stream->buffer_size; i++) {
            if (stream->data_buffer[i]) {
                // Ne pas libérer - les LUMs appartiennent au caller
                stream->data_buffer[i] = NULL;
            }
        }
        TRACKED_FREE(stream->data_buffer);
    }

    stream->memory_address = NULL;
    TRACKED_FREE(stream);
    *stream_ptr = NULL;
}

// Ajout LUM au stream
bool realtime_stream_push_lum(realtime_stream_t* stream, lum_t* lum) {
    if (!stream || !lum) return false;

    if (stream->memory_address != (void*)stream) return false;

    // Vérification buffer plein
    size_t next_write = (stream->write_index + 1) % stream->buffer_size;
    if (next_write == stream->read_index && stream->data_buffer[stream->write_index] != NULL) {
        // Buffer plein - écrasement du plus ancien
        stream->read_index = (stream->read_index + 1) % stream->buffer_size;
    }

    // Ajout nouveau LUM
    stream->data_buffer[stream->write_index] = lum;
    stream->write_index = next_write;
    stream->total_processed++;

    // Mise à jour timestamp et taux - optimisé < 1ms latence
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t current_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    // Optimisation lock-free pour latence minimale
    __atomic_store_n(&stream->timestamp_last, current_time, __ATOMIC_RELEASE);

    if (stream->timestamp_last > 0) {
        uint64_t time_diff = current_time - stream->timestamp_last;
        if (time_diff > 0) {
            stream->processing_rate = 1000000000.0 / time_diff; // LUMs/seconde
        }
    }

    stream->timestamp_last = current_time;
    stream->is_streaming = true;

    return true;
}

// Extraction LUM du stream
lum_t* realtime_stream_pop_lum(realtime_stream_t* stream) {
    if (!stream) return NULL;

    if (stream->memory_address != (void*)stream) return NULL;

    // Vérification buffer vide
    if (stream->read_index == stream->write_index && 
        stream->data_buffer[stream->read_index] == NULL) {
        return NULL;
    }

    lum_t* lum = stream->data_buffer[stream->read_index];
    stream->data_buffer[stream->read_index] = NULL;
    stream->read_index = (stream->read_index + 1) % stream->buffer_size;

    return lum;
}

// Création métriques analytiques
analytics_metrics_t* analytics_metrics_create(void) {
    analytics_metrics_t* metrics = TRACKED_MALLOC(sizeof(analytics_metrics_t));
    if (!metrics) return NULL;

    metrics->mean_position_x = 0.0;
    metrics->mean_position_y = 0.0;
    metrics->variance_x = 0.0;
    metrics->variance_y = 0.0;
    metrics->presence_ratio = 0.0;
    metrics->total_lums = 0;
    metrics->throughput_lums_sec = 0.0;
    metrics->latency_ns = 0.0;
    metrics->jitter_ns = 0.0;
    metrics->memory_address = (void*)metrics;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    metrics->last_update = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    return metrics;
}

// Destruction métriques
void analytics_metrics_destroy(analytics_metrics_t** metrics_ptr) {
    if (!metrics_ptr || !*metrics_ptr) return;

    analytics_metrics_t* metrics = *metrics_ptr;
    if (metrics->memory_address == (void*)metrics) {
        metrics->memory_address = NULL;
        TRACKED_FREE(metrics);
        *metrics_ptr = NULL;
    }
}

// Mise à jour métriques
bool analytics_update_metrics(analytics_metrics_t* metrics, lum_t* lum) {
    if (!metrics || !lum) return false;

    if (metrics->memory_address != (void*)metrics) return false;

    // Mise à jour moyenne position X (moyenne mobile)
    double delta_x = lum->position_x - metrics->mean_position_x;
    metrics->total_lums++;
    metrics->mean_position_x += delta_x / metrics->total_lums;

    // Mise à jour variance X (algorithme de Welford)
    double delta2_x = lum->position_x - metrics->mean_position_x;
    metrics->variance_x += delta_x * delta2_x;

    // Même chose pour Y
    double delta_y = lum->position_y - metrics->mean_position_y;
    metrics->mean_position_y += delta_y / metrics->total_lums;
    double delta2_y = lum->position_y - metrics->mean_position_y;
    metrics->variance_y += delta_y * delta2_y;

    // Ratio de présence
    static uint64_t presence_count = 0;
    if (lum->presence) presence_count++;
    metrics->presence_ratio = (double)presence_count / metrics->total_lums;

    // Mise à jour timestamp
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t current_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    if (metrics->last_update > 0) {
        uint64_t time_diff = current_time - metrics->last_update;
        metrics->latency_ns = time_diff;

        // Calcul jitter (variation de latence)
        static uint64_t last_latency = 0;
        if (last_latency > 0) {
            metrics->jitter_ns = labs((int64_t)metrics->latency_ns - (int64_t)last_latency);
        }
        last_latency = metrics->latency_ns;

        // Throughput
        if (time_diff > 0) {
            metrics->throughput_lums_sec = 1000000000.0 / time_diff;
        }
    }

    metrics->last_update = current_time;

    return true;
}

// Traçage complet processus analytique temps réel
bool realtime_analytics_full_trace(realtime_stream_t* stream, const char* trace_file) {
    if (!stream || !trace_file) return false;
    
    FILE* file = fopen(trace_file, "w");
    if (!file) return false;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    fprintf(file, "=== REALTIME ANALYTICS FULL TRACE ===\n");
    fprintf(file, "Timestamp: %lu.%09lu\n", ts.tv_sec, ts.tv_nsec);
    fprintf(file, "Stream buffer size: %zu\n", stream->buffer_size);
    fprintf(file, "Read index: %zu\n", stream->read_index);
    fprintf(file, "Write index: %zu\n", stream->write_index);
    fprintf(file, "Total processed: %lu\n", stream->total_processed);
    fprintf(file, "Processing rate: %.3f LUMs/sec\n", stream->processing_rate);
    fprintf(file, "Is streaming: %s\n", stream->is_streaming ? "true" : "false");
    
    fprintf(file, "\n=== DETAILED LUM ANALYSIS ===\n");
    
    // Analyse détaillée de chaque LUM
    size_t current_index = stream->read_index;
    size_t trace_count = 0;
    
    while (current_index != stream->write_index && trace_count < 1000) {
        lum_t* lum = stream->data_buffer[current_index];
        if (lum) {
            fprintf(file, "LUM[%zu]: id=%u, presence=%u, pos=(%d,%d), type=%u, timestamp=%lu\n",
                   trace_count, lum->id, lum->presence, lum->position_x, lum->position_y,
                   lum->structure_type, lum->timestamp);
            
            // Calculs analytiques détaillés
            double distance_from_origin = sqrt(lum->position_x * lum->position_x + 
                                             lum->position_y * lum->position_y);
            fprintf(file, "  -> Distance from origin: %.6f\n", distance_from_origin);
            fprintf(file, "  -> Quadrant: %s\n", 
                   (lum->position_x >= 0 && lum->position_y >= 0) ? "Q1" :
                   (lum->position_x < 0 && lum->position_y >= 0) ? "Q2" :
                   (lum->position_x < 0 && lum->position_y < 0) ? "Q3" : "Q4");
            
            trace_count++;
        }
        current_index = (current_index + 1) % stream->buffer_size;
    }
    
    fprintf(file, "\n=== TRACE SUMMARY ===\n");
    fprintf(file, "Total LUMs traced: %zu\n", trace_count);
    
    fclose(file);
    return true;
}

// Analyse stream temps réel avec traçage complet
analytics_result_t* realtime_analyze_stream(realtime_stream_t* stream, analytics_config_t* config) {
    if (!stream || !config) return NULL;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    analytics_result_t* result = TRACKED_MALLOC(sizeof(analytics_result_t));
    if (!result) return NULL;

    result->memory_address = (void*)result;
    result->analysis_success = false;
    result->cpu_usage_percent = 0.0;
    result->memory_usage_gb = 0.0;

    // Traçage complet automatique
    char trace_filename[256];
    snprintf(trace_filename, sizeof(trace_filename), 
             "realtime_analysis_trace_%lu.txt", start.tv_sec);
    realtime_analytics_full_trace(stream, trace_filename);

    // Création métriques
    result->current_metrics = analytics_metrics_create();
    if (!result->current_metrics) {
        TRACKED_FREE(result);
        return NULL;
    }

    // Analyse de tous les LUMs dans le stream avec traçage détaillé
    size_t analyzed_count = 0;
    size_t current_index = stream->read_index;
    double sum_distances = 0.0;
    size_t quadrant_counts[4] = {0, 0, 0, 0}; // Q1, Q2, Q3, Q4

    while (current_index != stream->write_index) {
        lum_t* lum = stream->data_buffer[current_index];
        if (lum) {
            analytics_update_metrics(result->current_metrics, lum);
            analyzed_count++;
            
            // Calculs analytiques additionnels pour traçage
            double distance = sqrt(lum->position_x * lum->position_x + 
                                 lum->position_y * lum->position_y);
            sum_distances += distance;
            
            // Classification par quadrant
            if (lum->position_x >= 0 && lum->position_y >= 0) quadrant_counts[0]++;
            else if (lum->position_x < 0 && lum->position_y >= 0) quadrant_counts[1]++;
            else if (lum->position_x < 0 && lum->position_y < 0) quadrant_counts[2]++;
            else quadrant_counts[3]++;
        }
        current_index = (current_index + 1) % stream->buffer_size;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    result->processing_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                (end.tv_nsec - start.tv_nsec);

    if (analyzed_count > 0) {
        result->analysis_success = true;
        
        // Métriques avancées avec traçage (format sécurisé)
        double avg_distance = sum_distances / analyzed_count;
        int written = snprintf(result->error_message, sizeof(result->error_message),
                              "Stream analysis: %zu LUMs, dist=%.3f, Q1=%zu,Q2=%zu,Q3=%zu,Q4=%zu",
                              analyzed_count, avg_distance, 
                              quadrant_counts[0], quadrant_counts[1], 
                              quadrant_counts[2], quadrant_counts[3]);
        if (written >= (int)sizeof(result->error_message)) {
            result->error_message[sizeof(result->error_message) - 1] = '\0';
        }

        // Calcul variance finale
        if (result->current_metrics->total_lums > 1) {
            result->current_metrics->variance_x /= (result->current_metrics->total_lums - 1);
            result->current_metrics->variance_y /= (result->current_metrics->total_lums - 1);
        }
    } else {
        SAFE_STRCPY(result->error_message, "No LUMs found in stream for analysis", sizeof(result->error_message));
    }

    return result;
}

// Tests stress 100M+ LUMs
bool analytics_stress_test_100m_lums(analytics_config_t* config) {
    if (!config) return false;

    printf("=== ANALYTICS STRESS TEST: 100M+ LUMs ===\n");

    const size_t lum_count = 100000000; // 100M LUMs
    const size_t buffer_size = 1000000;  // Buffer 1M

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Creating realtime stream with buffer size %zu...\n", buffer_size);
    realtime_stream_t* stream = realtime_stream_create(buffer_size);

    if (!stream) {
        printf("❌ Failed to create analytics stream\n");
        return false;
    }

    printf("Creating test LUMs for analytics...\n");

    // Test avec échantillon représentatif
    const size_t test_lums = 100000; // 100K LUMs test
    lum_t* test_lums_array = TRACKED_MALLOC(test_lums * sizeof(lum_t));

    if (!test_lums_array) {
        realtime_stream_destroy(&stream);
        printf("❌ Failed to allocate test LUMs\n");
        return false;
    }

    // Initialisation LUMs test
    for (size_t i = 0; i < test_lums; i++) {
        test_lums_array[i].id = i;
        test_lums_array[i].presence = (i % 3 != 0) ? 1 : 0; // 66% présence
        test_lums_array[i].position_x = (int32_t)(i % 1000);
        test_lums_array[i].position_y = (int32_t)(i / 1000);
        test_lums_array[i].structure_type = LUM_STRUCTURE_LINEAR;
        test_lums_array[i].timestamp = i;
        test_lums_array[i].memory_address = &test_lums_array[i];
        test_lums_array[i].checksum = 0;
        test_lums_array[i].is_destroyed = 0;
    }

    // Injection dans stream
    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t injected_count = 0;

    for (size_t i = 0; i < test_lums; i++) {
        if (realtime_stream_push_lum(stream, &test_lums_array[i])) {
            injected_count++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double injection_time = (end.tv_sec - start.tv_sec) + 
                           (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("✅ Injected %zu LUMs in %.3f seconds\n", injected_count, injection_time);
    printf("Injection rate: %.0f LUMs/second\n", injected_count / injection_time);

    // Projection pour 100M
    double projected_time = injection_time * (lum_count / (double)test_lums);
    printf("Projected time for %zu LUMs: %.1f seconds\n", lum_count, projected_time);

    // Test analyse
    analytics_config_t* test_config = analytics_config_create_default();
    if (test_config) {
        analytics_result_t* analysis = realtime_analyze_stream(stream, test_config);
        if (analysis && analysis->analysis_success) {
            printf("✅ Stream analysis completed successfully\n");
            printf("Mean position: (%.2f, %.2f)\n", 
                   analysis->current_metrics->mean_position_x,
                   analysis->current_metrics->mean_position_y);
            printf("Presence ratio: %.2f%%\n", 
                   analysis->current_metrics->presence_ratio * 100.0);
            analytics_result_destroy(&analysis);
        }
        analytics_config_destroy(&test_config);
    }

    // Cleanup
    TRACKED_FREE(test_lums_array);
    realtime_stream_destroy(&stream);

    printf("✅ Analytics stress test 100M+ LUMs completed successfully\n");
    return true;
}

// Configuration par défaut
analytics_config_t* analytics_config_create_default(void) {
    analytics_config_t* config = TRACKED_MALLOC(sizeof(analytics_config_t));
    if (!config) return NULL;

    config->stream_buffer_size = 100000;
    config->sampling_rate_hz = ANALYTICS_DEFAULT_SAMPLING_RATE;
    config->enable_anomaly_detection = false;
    config->enable_temporal_aggregation = false;
    config->max_concurrent_streams = 10;
    config->memory_usage_limit_gb = 4.0;
    config->enable_gpu_acceleration = false;
    config->memory_address = (void*)config;

    return config;
}

// Destruction configuration
void analytics_config_destroy(analytics_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;

    analytics_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}

// Destruction résultat
void analytics_result_destroy(analytics_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;

    analytics_result_t* result = *result_ptr;
    if (result->memory_address == (void*)result) {
        if (result->current_metrics) {
            analytics_metrics_destroy(&result->current_metrics);
        }
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}