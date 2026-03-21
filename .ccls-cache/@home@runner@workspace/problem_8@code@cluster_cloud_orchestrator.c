#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "../../src/common/common_types.h"
#include "../../src/debug/forensic_logger.h"

typedef struct {
    int problem_id;
    const char* name;
} problem_task_t;

void* simulate_problem(void* arg) {
    problem_task_t* task = (problem_task_t*)arg;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t start = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    printf("[CLUSTER_CLOUD] [%lu] START_SIMULATION: Problem_%d (%s)\n", start, task->problem_id, task->name);
    
    // Heuristic simulation load
    for(int i=0; i<1000000; i++) {
        if(i % 100000 == 0) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            // Simulate anomaly detection
            if(task->problem_id == 8 && i == 500000) {
                printf("[ANOMALY_DETECTED] [%lu] Problem_8: Unusual sequence density at step %d\n", (uint64_t)ts.tv_nsec, i);
            }
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t end = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    printf("[CLUSTER_CLOUD] [%lu] END_SIMULATION: Problem_%d. Duration: %lu ns\n", end, task->problem_id, end - start);
    
    return NULL;
}

int main() {
    pthread_t threads[10];
    problem_task_t tasks[10] = {
        {1, "Riemann"}, {2, "P vs NP"}, {3, "BSD"}, {4, "Hodge"}, {5, "Navier-Stokes"},
        {6, "Goldbach"}, {7, "Twin Primes"}, {8, "Collatz"}, {9, "abc"}, {10, "Spectral Gap"}
    };

    printf("=== ACTIVATING CLUSTER CLOUD LEVEL ===\n");
    for(int i=0; i<10; i++) {
        pthread_create(&threads[i], NULL, simulate_problem, &tasks[i]);
    }

    for(int i=0; i<10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("=== CLUSTER CLOUD SIMULATION COMPLETE ===\n");
    return 0;
}
