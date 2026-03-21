#ifndef FORENSIC_LOGGER_H
#define FORENSIC_LOGGER_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include "../common/common_types.h"

// Using unified forensic levels from common_types.h
typedef unified_forensic_level_e forensic_level_e;

#include "../lum/lum_core.h"

// Macros timing différenciées selon usage
#define FORENSIC_TIMING_START(timer_var) \
    struct timespec timer_var##_start, timer_var##_end; \
    clock_gettime(CLOCK_MONOTONIC, &timer_var##_start)

#define FORENSIC_TIMING_END(timer_var) \
    clock_gettime(CLOCK_MONOTONIC, &timer_var##_end)

#define FORENSIC_TIMING_CALC_NS(timer_var) \
    ((timer_var##_end.tv_sec - timer_var##_start.tv_sec) * 1000000000ULL + \
     (timer_var##_end.tv_nsec - timer_var##_start.tv_nsec))

#define FILE_TIMESTAMP_GET() \
    ({ \
        struct timespec ts; \
        clock_gettime(CLOCK_REALTIME, &ts); \
        ts.tv_sec * 1000000000ULL + ts.tv_nsec; \
    })

bool forensic_logger_init(const char* filename);
bool forensic_logger_init_individual_files(void);
void forensic_log_memory_operation(const char* operation, void* ptr, size_t size);
void forensic_log_lum_operation(const char* operation, uint64_t lum_count, double duration_ns);
void forensic_log_individual_lum(uint32_t lum_id, const char* operation, uint64_t timestamp_ns);
void forensic_logger_destroy(void);

// General forensic logging function
void forensic_log(forensic_level_e level, const char* function, const char* format, ...);

#endif // FORENSIC_LOGGER_H