#ifndef TRANSACTION_WAL_EXTENSION_H
#define TRANSACTION_WAL_EXTENSION_H

#include "data_persistence.h"
#include <stdatomic.h>
#include <pthread.h>

// EXTENSION de transaction_record_t existant (pas de duplication)
typedef struct {
    transaction_record_t base_record;  // Utilise l'existant
    uint32_t wal_magic_signature;     // Extension: 0x57414C58 "WALX"
    uint16_t wal_version;             // Extension: version WAL
    uint64_t sequence_number_global;   // Extension: séquence globale
    uint64_t nanosecond_timestamp;    // Extension: timestamp précis
    uint32_t data_integrity_crc32;    // Extension: CRC32 données
    uint32_t header_integrity_crc32;  // Extension: CRC32 header
    uint8_t reserved_expansion[16];   // Extension: padding futur
    uint32_t magic_number;            // Magic number pour validation
    size_t lum_count;                 // Nombre de LUMs dans la transaction
} transaction_wal_extended_t;

// EXTENSION du contexte persistance existant
typedef struct {
    persistence_context_t* base_context;  // Réutilise l'existant
    char wal_extension_filename[256];
    FILE* wal_extension_file;
    atomic_uint_fast64_t sequence_counter_atomic;
    atomic_uint_fast64_t transaction_counter_atomic;
    bool auto_fsync_enabled;
    pthread_mutex_t wal_extension_mutex;
    bool recovery_mode_active;
    uint32_t magic_number;
    transaction_wal_extended_t* transactions;
    size_t transaction_count;
    size_t checkpoint_count;
    size_t last_checkpoint_transaction;
} wal_extension_context_t;

// EXTENSION des résultats existants
typedef struct {
    storage_result_t* base_result;     // Réutilise storage_result_t
    uint64_t wal_sequence_assigned;
    uint64_t wal_transaction_id;
    bool wal_durability_confirmed;
    char wal_error_details[256];
    uint32_t magic_number;
} wal_extension_result_t;

// API Extension WAL (pas de duplication de noms)
wal_extension_context_t* wal_extension_context_create(const char* wal_filename);
void wal_extension_context_destroy(wal_extension_context_t* ctx);

wal_extension_result_t* wal_extension_begin_transaction(wal_extension_context_t* ctx);
wal_extension_result_t* wal_extension_commit_transaction(wal_extension_context_t* ctx, uint64_t transaction_id);
wal_extension_result_t* wal_extension_rollback_transaction(wal_extension_context_t* ctx, uint64_t transaction_id);

wal_extension_result_t* wal_extension_log_lum_operation(wal_extension_context_t* ctx, 
                                                       uint64_t transaction_id,
                                                       const lum_t* lum);

// Fonctions support recovery manager
bool wal_extension_replay_from_existing_persistence(wal_extension_context_t* wal_ctx, 
                                                   void* persistence_ctx);
bool wal_extension_create_checkpoint_with_existing(wal_extension_context_t* wal_ctx,
                                                   void* persistence_ctx);
bool wal_extension_verify_integrity_complete(wal_extension_context_t* ctx);

#endif // TRANSACTION_WAL_EXTENSION_H