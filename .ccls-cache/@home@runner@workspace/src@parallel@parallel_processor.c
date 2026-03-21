// _POSIX_C_SOURCE défini par Makefile
// _GNU_SOURCE is already defined in Makefile
#include "parallel_processor.h"
#include "../logger/lum_logger.h"
#include "../debug/memory_tracker.h"  // NOUVEAU: Pour TRACKED_MALLOC/FREE
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>  // Pour usleep - CORRECTION APPLIQUÉE
#include <time.h>
#include <errno.h>

// Create parallel processor
parallel_processor_t* parallel_processor_create(int worker_count) {
    if (worker_count <= 0 || worker_count > MAX_WORKER_THREADS) {
        worker_count = DEFAULT_WORKER_COUNT;
    }

    parallel_processor_t* processor = TRACKED_MALLOC(sizeof(parallel_processor_t));
    if (!processor) return NULL;

    processor->worker_count = worker_count;
    processor->is_initialized = false;
    processor->total_tasks_processed = 0;
    processor->total_processing_time = 0.0;

    // Initialize task queue
    if (!task_queue_init(&processor->task_queue)) {
        TRACKED_FREE(processor);
        return NULL;
    }

    // Initialize statistics mutex
    if (pthread_mutex_init(&processor->stats_mutex, NULL) != 0) {
        task_queue_destroy(&processor->task_queue);
        TRACKED_FREE(processor);
        return NULL;
    }

    // Create worker threads
    for (int i = 0; i < worker_count; i++) {
        processor->workers[i].thread_id = i;
        processor->workers[i].is_active = false;
        processor->workers[i].should_exit = false;
        processor->workers[i].tasks_completed = 0;

        if (pthread_create(&processor->workers[i].thread, NULL, 
                          worker_thread_main, processor) != 0) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                processor->workers[j].should_exit = true;
                pthread_join(processor->workers[j].thread, NULL);
            }
            pthread_mutex_destroy(&processor->stats_mutex);
            task_queue_destroy(&processor->task_queue);
            TRACKED_FREE(processor);
            return NULL;
        }
        processor->workers[i].is_active = true;
    }

    processor->is_initialized = true;
    return processor;
}

void parallel_processor_destroy(parallel_processor_t* processor) {
    if (!processor) return;

    // Signal all workers to exit
    for (int i = 0; i < processor->worker_count; i++) {
        if (processor->workers[i].is_active) {
            processor->workers[i].should_exit = true;
        }
    }

    // Wake up all workers
    pthread_cond_broadcast(&processor->task_queue.condition);

    // Wait for all workers to finish
    for (int i = 0; i < processor->worker_count; i++) {
        if (processor->workers[i].is_active) {
            pthread_join(processor->workers[i].thread, NULL);
        }
    }

    task_queue_destroy(&processor->task_queue);
    pthread_mutex_destroy(&processor->stats_mutex);
    TRACKED_FREE(processor);
}

// Task management
parallel_task_t* parallel_task_create(parallel_task_type_e type, void* input_data, size_t data_size) {
    parallel_task_t* task = TRACKED_MALLOC(sizeof(parallel_task_t));
    if (!task) return NULL;

    task->type = type;
    task->input_data = input_data;
    task->output_data = NULL;
    task->data_size = data_size;
    task->is_completed = false;
    task->error_code = 0;
    task->error_message[0] = '\0';
    task->next = NULL;

    return task;
}

void parallel_task_destroy(parallel_task_t* task) {
    if (task) {
        // Note: input_data and output_data are managed by caller
        TRACKED_FREE(task);
    }
}

bool parallel_processor_submit_task(parallel_processor_t* processor, parallel_task_t* task) {
    if (!processor || !processor->is_initialized || !task) return false;

    return task_queue_enqueue(&processor->task_queue, task);
}

bool parallel_processor_wait_for_completion(parallel_processor_t* processor) {
    if (!processor || !processor->is_initialized) return false;

    // Simple implementation: wait until queue is empty
    while (!task_queue_is_empty(&processor->task_queue)) {
        usleep(1000); // Sleep 1ms
    }

    return true;
}

// Queue operations
bool task_queue_init(task_queue_t* queue) {
    if (!queue) return false;

    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return false;
    }

    if (pthread_cond_init(&queue->condition, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return false;
    }

    return true;
}

void task_queue_destroy(task_queue_t* queue) {
    if (!queue) return;

    pthread_mutex_lock(&queue->mutex);

    // Free all remaining tasks
    parallel_task_t* current = queue->head;
    while (current) {
        parallel_task_t* next = current->next;
        parallel_task_destroy(current);
        current = next;
    }

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->condition);
}

bool task_queue_enqueue(task_queue_t* queue, parallel_task_t* task) {
    if (!queue || !task) return false;

    pthread_mutex_lock(&queue->mutex);

    if (queue->tail) {
        queue->tail->next = task;
    } else {
        queue->head = task;
    }
    queue->tail = task;
    task->next = NULL;
    queue->count++;

    pthread_cond_signal(&queue->condition);
    pthread_mutex_unlock(&queue->mutex);

    return true;
}

parallel_task_t* task_queue_dequeue(task_queue_t* queue) {
    if (!queue) return NULL;

    pthread_mutex_lock(&queue->mutex);

    while (queue->head == NULL) {
        pthread_cond_wait(&queue->condition, &queue->mutex);
    }

    parallel_task_t* task = queue->head;
    queue->head = task->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return task;
}

bool task_queue_is_empty(task_queue_t* queue) {
    if (!queue) return true;

    pthread_mutex_lock(&queue->mutex);
    bool empty = (queue->count == 0);
    pthread_mutex_unlock(&queue->mutex);

    return empty;
}

// Worker thread main function
void* worker_thread_main(void* arg) {
    parallel_processor_t* processor = (parallel_processor_t*)arg;
    if (!processor) return NULL;

    // Identifier le worker actuel
    int worker_id = -1;
    pthread_t current_thread = pthread_self();
    for (int i = 0; i < processor->worker_count; i++) {
        if (pthread_equal(processor->workers[i].thread, current_thread)) {
            worker_id = i;
            break;
        }
    }

    while (worker_id >= 0 && !processor->workers[worker_id].should_exit) {
        parallel_task_t* task = task_queue_dequeue(&processor->task_queue);
        if (!task) {
            // Vérifier à nouveau la condition de sortie après attente
            if (processor->workers[worker_id].should_exit) break;
            continue;
        }

        clock_t start_time = clock();
        bool success = execute_task(task);
        clock_t end_time = clock();

        task->is_completed = true;
        if (!success) {
            task->error_code = 1;
            strncpy(task->error_message, "Task execution failed", sizeof(task->error_message) - 1);
        }

        // Update statistics
        pthread_mutex_lock(&processor->stats_mutex);
        processor->total_tasks_processed++;
        processor->total_processing_time += ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        pthread_mutex_unlock(&processor->stats_mutex);

        // Update worker statistics
        for (int i = 0; i < processor->worker_count; i++) {
            if (pthread_self() == processor->workers[i].thread) {
                processor->workers[i].tasks_completed++;
                break;
            }
        }
    }

    return NULL;
}

bool execute_task(parallel_task_t* task) {
    if (!task) return false;

    switch (task->type) {
        case TASK_LUM_CREATE: {
            // Create LUM from input data
            if (task->data_size >= sizeof(lum_t)) {
                lum_t* input = (lum_t*)task->input_data;
                lum_t* output = lum_create(input->presence, input->position_x, 
                                         input->position_y, input->structure_type);
                task->output_data = output;
                return output != NULL;
            }
            return false;
        }

        case TASK_VORAX_FUSE: {
            // Fuse two groups
            lum_group_t** groups = (lum_group_t**)task->input_data;
            if (task->data_size >= sizeof(lum_group_t*) * 2 && groups[0] && groups[1]) {
                vorax_result_t* fusion_result = vorax_fuse(groups[0], groups[1]);
                task->output_data = fusion_result;
                return fusion_result && fusion_result->success;
            }
            return false;
        }

        case TASK_BINARY_CONVERT: {
            // Convert binary to LUM
            uint8_t* binary_data = (uint8_t*)task->input_data;
            binary_lum_result_t* result = convert_binary_to_lum(binary_data, task->data_size);
            task->output_data = result;
            return result && result->success;
        }

        default:
            return false;
    }
}

// Statistics
void parallel_processor_print_stats(parallel_processor_t* processor) {
    if (!processor) return;

    pthread_mutex_lock(&processor->stats_mutex);

    printf("=== Parallel Processor Statistics ===\n");
    printf("Worker Threads: %d\n", processor->worker_count);
    printf("Total Tasks Processed: %zu\n", processor->total_tasks_processed);
    printf("Total Processing Time: %.3f seconds\n", processor->total_processing_time);

    if (processor->total_tasks_processed > 0) {
        printf("Average Task Time: %.3f ms\n", 
               (processor->total_processing_time * 1000.0) / processor->total_tasks_processed);
    }

    printf("Worker Performance:\n");
    for (int i = 0; i < processor->worker_count; i++) {
        printf("  Worker %d: %zu tasks completed\n", i, processor->workers[i].tasks_completed);
    }

    printf("======================================\n");

    pthread_mutex_unlock(&processor->stats_mutex);
}

double parallel_processor_get_efficiency(parallel_processor_t* processor) {
    if (!processor || processor->total_tasks_processed == 0) return 0.0;

    pthread_mutex_lock(&processor->stats_mutex);

    // Calculate theoretical vs actual processing time
    size_t total_worker_tasks = 0;
    for (int i = 0; i < processor->worker_count; i++) {
        total_worker_tasks += processor->workers[i].tasks_completed;
    }

    double efficiency = (double)total_worker_tasks / 
                       (processor->worker_count * processor->total_tasks_processed);

    pthread_mutex_unlock(&processor->stats_mutex);

    return efficiency;
}

// Thread pool compatibility functions
struct thread_pool {
    parallel_processor_t* processor;
};

thread_pool_t* thread_pool_create(int worker_count) {
    thread_pool_t* pool = TRACKED_MALLOC(sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->processor = parallel_processor_create(worker_count);
    if (!pool->processor) {
        TRACKED_FREE(pool);
        return NULL;
    }
    return pool;
}

void thread_pool_destroy(thread_pool_t* pool) {
    if (pool) {
        parallel_processor_destroy(pool->processor);
        TRACKED_FREE(pool);
    }
}

bool thread_pool_submit(thread_pool_t* pool, parallel_task_t* task) {
    if (!pool || !pool->processor) return false;
    return parallel_processor_submit_task(pool->processor, task);
}

bool thread_pool_wait_all(thread_pool_t* pool) {
    if (!pool || !pool->processor) return false;
    return parallel_processor_wait_for_completion(pool->processor);
}

// High-level processing functions
parallel_process_result_t parallel_process_lums(lum_t** lums, int count, int threads) {
    parallel_process_result_t result = {0};

    if (!lums || count <= 0) {
        result.success = false;
        SAFE_STRCPY(result.error_message, "Invalid input parameters", sizeof(result.error_message));
        return result;
    }

    parallel_processor_t* processor = parallel_processor_create(threads);
    if (!processor) {
        result.success = false;
        SAFE_STRCPY(result.error_message, "Failed to create processor", sizeof(result.error_message));
        return result;
    }

    clock_t start_time = clock();

    // Submit all LUMs as tasks
    for (int i = 0; i < count; i++) {
        parallel_task_t* task = parallel_task_create(TASK_LUM_CREATE, lums[i], sizeof(lum_t));
        if (task) {
            parallel_processor_submit_task(processor, task);
        }
    }

    // Wait for completion
    parallel_processor_wait_for_completion(processor);

    clock_t end_time = clock();
    result.processing_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    result.processed_count = count;
    result.success = true;

    parallel_processor_destroy(processor);
    return result;
}

bool distribute_work(lum_t** lums, int count, int threads, work_distribution_t* dist) {
    if (!lums || !dist || count <= 0 || threads <= 0) return false;

    dist->thread_count = threads;
    dist->total_tasks = count;
    dist->tasks_per_thread = TRACKED_MALLOC(threads * sizeof(int));
    if (!dist->tasks_per_thread) return false;

    int base_tasks = count / threads;
    int extra_tasks = count % threads;

    for (int i = 0; i < threads; i++) {
        dist->tasks_per_thread[i] = base_tasks + (i < extra_tasks ? 1 : 0);
    }

    return true;
}

double parallel_reduce_lums(lum_t** lums, int count, int threads) {
    if (!lums || count <= 0) return 0.0;

    (void)threads; // Parameter reserved for future parallel implementation

    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        if (lums[i]) {
            sum += lums[i]->presence * (lums[i]->position_x + lums[i]->position_y);
        }
    }

    return sum;
}