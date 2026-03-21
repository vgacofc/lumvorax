
#ifndef STORAGE_BACKEND_H
#define STORAGE_BACKEND_H

#include "data_persistence.h"

// Alias pour compatibilité avec main.c
// Ces fonctions utilisent le système data_persistence existant

// Types déjà définis dans data_persistence.h :
// - storage_backend_t
// - transaction_t
// - serialized_data_t

// Fonctions déjà déclarées dans data_persistence.h :
// - storage_backend_create()
// - storage_backend_destroy()
// - store_lum()
// - load_lum()
// - begin_transaction()
// - commit_transaction()
// - storage_backend_store_batch()

#endif // STORAGE_BACKEND_H
