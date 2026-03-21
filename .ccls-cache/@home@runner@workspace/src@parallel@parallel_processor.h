#ifndef PARALLEL_PROCESSOR_H
#define PARALLEL_PROCESSOR_H

#include "../lum/lum_core.h"
#include "../binary/binary_lum_converter.h"
#include "../vorax/vorax_operations.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>

#define MAX_WORKER_THREADS 16
#define DEFAULT_WORKER_COUNT 4

// Task types for parallel processing
typedef enum {
    TASK_LUM_CREATE,
    TASK_GROUP_OPERATION,
    TASK_VORAX_FUSE,
    TASK_VORAX_SPLIT,
    TASK_BINARY_CONVERT,
    TASK_CUSTOM
} parallel_task_type_e;

// Task structure
typedef struct parallel_task {
    parallel_task_type_e type;
    void* input_data;
    void* output_data;
    size_t data_size;
    bool is_completed;
    int error_code;
    char error_message[256];
    struct parallel_task* next;
} parallel_task_t;

// Task queue
typedef struct {
    parallel_task_t* head;
    parallel_task_t* tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} task_queue_t;

// Worker thread info
typedef struct {
    pthread_t thread;
    int thread_id;
    bool is_active;
    bool should_exit;
    size_t tasks_completed;
} worker_thread_t;

// Parallel processor context
typedef struct {
    worker_thread_t workers[MAX_WORKER_THREADS];
    int worker_count;
    task_queue_t task_queue;
    bool is_initialized;
    size_t total_tasks_processed;
    double total_processing_time;
    pthread_mutex_t stats_mutex;
} parallel_processor_t;

// Function declarations
parallel_processor_t* parallel_processor_create(int worker_count);
void parallel_processor_destroy(parallel_processor_t* processor);

// Task management
parallel_task_t* parallel_task_create(parallel_task_type_e type, void* input_data, size_t data_size);
void parallel_task_destroy(parallel_task_t* task);
bool parallel_processor_submit_task(parallel_processor_t* processor, parallel_task_t* task);
bool parallel_processor_wait_for_completion(parallel_processor_t* processor);

// Queue operations
bool task_queue_init(task_queue_t* queue);
void task_queue_destroy(task_queue_t* queue);
bool task_queue_enqueue(task_queue_t* queue, parallel_task_t* task);
parallel_task_t* task_queue_dequeue(task_queue_t* queue);
bool task_queue_is_empty(task_queue_t* queue);

// Parallel operations
bool parallel_process_lum_group(parallel_processor_t* processor, lum_group_t* group);
bool parallel_vorax_operations(parallel_processor_t* processor, 
                              lum_group_t** groups, size_t group_count);
bool parallel_binary_conversions(parallel_processor_t* processor, 
                                 void** data_array, size_t array_count);

// Statistics
void parallel_processor_print_stats(parallel_processor_t* processor);
double parallel_processor_get_efficiency(parallel_processor_t* processor);

// Internal worker functions
void* worker_thread_main(void* arg);
bool execute_task(parallel_task_t* task);

// Thread pool compatibility layer
typedef struct thread_pool thread_pool_t;

// Result structures for high-level operations
typedef struct {
    bool success;
    int processed_count;
    double processing_time;
    char error_message[256];
} parallel_process_result_t;

typedef struct {
    int thread_count;
    int total_tasks;
    int* tasks_per_thread;
} work_distribution_t;

// Thread pool functions
thread_pool_t* thread_pool_create(int worker_count);
void thread_pool_destroy(thread_pool_t* pool);
bool thread_pool_submit(thread_pool_t* pool, parallel_task_t* task);
bool thread_pool_wait_all(thread_pool_t* pool);

// High-level processing functions
parallel_process_result_t parallel_process_lums(lum_t** lums, int count, int threads);
bool distribute_work(lum_t** lums, int count, int threads, work_distribution_t* dist);
double parallel_reduce_lums(lum_t** lums, int count, int threads);

#endif // PARALLEL_PROCESSOR_H