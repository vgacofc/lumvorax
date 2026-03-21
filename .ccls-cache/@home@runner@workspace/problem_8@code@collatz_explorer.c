#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../../src/common/common_types.h"
#include "../../src/debug/forensic_logger.h"

int main() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t start = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    printf("[RESEARCH_LOG] [%lu] START_EXPLORATION: Problem_8_Collatz\n", start);
    
    // Simulation simple pour logging
    uint64_t n = 7;
    while (n != 1) {
        if (n % 2 == 0) n /= 2;
        else n = 3 * n + 1;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[RESEARCH_LOG] [%lu] STEP_VALUE: %lu\n", (uint64_t)ts.tv_nsec, n);
    }
    
    return 0;
}
