#ifndef ASYNC_IO_MANAGER_H
#define ASYNC_IO_MANAGER_H

#include "../persistence/data_persistence.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct {
    persistence_context_t* ctx;
    pthread_t io_thread;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    bool running;
    // File d'attente simplifiée pour le WAL
} async_io_manager_t;

async_io_manager_t* async_io_init(persistence_context_t* ctx);
void async_io_shutdown(async_io_manager_t* manager);
bool async_io_enqueue_save(async_io_manager_t* manager, const lum_t* lum, const char* filename);

#endif
