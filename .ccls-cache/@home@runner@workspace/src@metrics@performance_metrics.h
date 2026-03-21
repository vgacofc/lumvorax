#ifndef PERFORMANCE_METRICS_H_INCLUDED
#define PERFORMANCE_METRICS_H_INCLUDED

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_METRIC_NAME_LENGTH 64
#define MAX_METRICS_COUNT 100
#define MAX_PERFORMANCE_COUNTERS 50

// Metric types
typedef enum {
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM,
    METRIC_TIMER
} metric_type_e;

// Individual performance counter
typedef struct {
    char name[MAX_METRIC_NAME_LENGTH];
    metric_type_e type;
    double value;
    double min_value;
    double max_value;
    double sum_value;
    uint64_t count;
    struct timespec last_updated;
    bool is_active;
} performance_counter_t;

// Individual metric (for compatibility)
typedef performance_counter_t performance_metric_t;

// Histogram bucket
typedef struct {
    double upper_bound;
    uint64_t count;
} histogram_bucket_t;

// Performance metrics context
typedef struct {
    performance_counter_t counters[MAX_PERFORMANCE_COUNTERS];
    size_t metric_count;
    bool is_initialized;
    struct timespec start_time;
    uint64_t total_operations;
    time_t last_update;
    size_t memory_peak;
    double cpu_peak;
    size_t operation_count;
    size_t error_count;
    double cpu_usage;
    size_t memory_usage;
    size_t peak_memory;
} performance_metrics_t;

// Timer context for measuring operations
typedef struct {
    struct timespec start_time;
    struct timespec end_time;
    bool is_running;
    double total_elapsed;
    double elapsed_seconds;
} operation_timer_t;

// Benchmark result structure
typedef struct {
    int iterations;
    double total_time;
    double average_time;
    double min_time;
    double max_time;
    double operations_per_second;
} benchmark_result_t;

// Throughput calculator structure
typedef struct {
    size_t total_operations;
    struct timespec start_time;
    struct timespec last_update;
    double current_throughput;
    double peak_throughput;
} throughput_calculator_t;

// Function declarations
performance_metrics_t* performance_metrics_create(void);
void performance_metrics_destroy(performance_metrics_t* metrics);

// Metric registration and management
bool performance_metrics_register(performance_metrics_t* ctx, 
                                 const char* name, metric_type_e type);
performance_metric_t* performance_metrics_get(performance_metrics_t* ctx, const char* name);

// Metric operations
bool performance_metrics_increment_counter(performance_metrics_t* ctx, const char* name, double value);
bool performance_metrics_set_gauge(performance_metrics_t* ctx, const char* name, double value);
bool performance_metrics_record_histogram(performance_metrics_t* ctx, const char* name, double value);

// Timer operations
operation_timer_t* operation_timer_create(void);
void operation_timer_destroy(operation_timer_t* timer);
bool operation_timer_start(operation_timer_t* timer);
bool operation_timer_stop(operation_timer_t* timer);
double operation_timer_get_elapsed(operation_timer_t* timer);

// High-level timing functions
bool performance_metrics_start_timer(performance_metrics_t* ctx, const char* name);
bool performance_metrics_stop_timer(performance_metrics_t* ctx, const char* name);

// System metrics
void performance_metrics_update_system_stats(performance_metrics_t* ctx);
double performance_metrics_get_cpu_usage(void);
size_t performance_metrics_get_memory_usage(void);

// Reporting and analysis
void performance_metrics_print_summary(performance_metrics_t* ctx);
void performance_metrics_print_detailed(performance_metrics_t* ctx);
bool performance_metrics_export_csv(performance_metrics_t* ctx, const char* filename);
bool performance_metrics_export_json(performance_metrics_t* ctx, const char* filename);

// Statistical analysis
double performance_metrics_calculate_mean(performance_metric_t* metric);
double performance_metrics_calculate_stddev(performance_metric_t* metric);
double performance_metrics_calculate_percentile(performance_metric_t* metric, double percentile);

// Benchmarking utilities
bool performance_metrics_benchmark_operation(performance_metrics_t* ctx, 
                                           const char* name, 
                                           void (*operation)(void*), 
                                           void* data,
                                           int iterations);

// Benchmarking functions
benchmark_result_t* performance_benchmark_function(void (*func)(void), int iterations);
void benchmark_result_destroy(benchmark_result_t* result);

// Throughput calculator functions
throughput_calculator_t* throughput_calculator_create(void);
void throughput_calculator_destroy(throughput_calculator_t* calc);
void throughput_calculator_add_operations(throughput_calculator_t* calc, size_t operations);
double throughput_calculator_get_current(throughput_calculator_t* calc);
double throughput_calculator_get_average(throughput_calculator_t* calc);
double throughput_calculator_get_peak(throughput_calculator_t* calc);

// Performance counter functions (for tests)
typedef struct {
    struct timespec start_time;
    bool is_running;
} test_performance_counter_t;

// Memory footprint structure for tests (conforme STANDARD_NAMES.md)
typedef struct {
    size_t heap_usage;
    size_t stack_usage;
    size_t peak_heap;
    size_t peak_stack;
    size_t allocation_count;
    size_t deallocation_count;
} memory_footprint_t;

performance_counter_t* performance_counter_create(void);
void performance_counter_destroy(performance_counter_t* counter);
void performance_counter_start(performance_counter_t* counter);
double performance_counter_stop(performance_counter_t* counter);

memory_footprint_t* memory_footprint_create(void);
void memory_footprint_destroy(memory_footprint_t* footprint);
void memory_footprint_update(memory_footprint_t* footprint);

// Utility functions
double timespec_to_seconds(struct timespec* ts);
void get_current_timespec(struct timespec* ts);
bool is_metric_name_valid(const char* name);

// Performance monitoring functions
void performance_start_timer(void);
uint64_t performance_get_elapsed_microseconds(void);

// Métriques LUM conversions (conforme STANDARD_NAMES 2025-01-10)
uint64_t convert_lums_per_second_to_bits_per_second(uint64_t lums_per_second);
double convert_lums_per_second_to_gigabits_per_second(uint64_t lums_per_second);
uint64_t calculate_authentic_lum_throughput(uint64_t lum_count, uint64_t microseconds_elapsed);

#endif /* PERFORMANCE_METRICS_H_INCLUDED */