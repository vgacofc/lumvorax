
#ifndef RECOVERY_MANAGER_EXTENSION_H
#define RECOVERY_MANAGER_EXTENSION_H

#include "data_persistence.h"
#include "transaction_wal_extension.h"

#define CRASH_DETECTION_EXTENSION_FILE ".lum_crash_detection_ext"
#define RECOVERY_STATE_EXTENSION_FILE ".lum_recovery_state_ext"

typedef enum {
    RECOVERY_STATE_NORMAL_EXTENDED,
    RECOVERY_STATE_CRASHED_DETECTED,
    RECOVERY_STATE_RECOVERING_ACTIVE,
    RECOVERY_STATE_RECOVERED_SUCCESS,
    RECOVERY_STATE_FAILED_EXTENDED
} recovery_state_extension_e;

// EXTENSION des informations recovery (utilise les types existants)
typedef struct {
    recovery_state_extension_e state;
    uint64_t crash_timestamp_nanoseconds;
    uint64_t recovery_timestamp_nanoseconds;
    uint32_t recovery_attempts_count;
    uint64_t last_checkpoint_sequence;
    char wal_extension_filename[256];
    char persistence_directory[256];
    char error_details_extended[512];
    bool auto_recovery_enabled;
    uint32_t magic_number;
} recovery_info_extension_t;

// EXTENSION du manager recovery (réutilise les contextes existants)
typedef struct {
    persistence_context_t* base_persistence_ctx;    // Réutilise existant
    wal_extension_context_t* wal_extension_ctx;     // Extension WAL
    char data_directory_path[256];
    char wal_extension_filename[256];
    bool auto_recovery_enabled;
    uint32_t max_recovery_attempts;
    recovery_info_extension_t* current_recovery_info;
    uint32_t magic_number;
} recovery_manager_extension_t;

// API Recovery Extension (pas de duplication)
recovery_manager_extension_t* recovery_manager_extension_create(const char* data_directory, 
                                                               const char* wal_filename);
void recovery_manager_extension_destroy(recovery_manager_extension_t* manager);

// Détection crash extension
bool recovery_manager_extension_detect_previous_crash(recovery_manager_extension_t* manager);
bool recovery_manager_extension_mark_clean_shutdown(recovery_manager_extension_t* manager);
bool recovery_manager_extension_mark_startup_begin(recovery_manager_extension_t* manager);

// Recovery automatique extension
bool recovery_manager_extension_auto_recover_complete(recovery_manager_extension_t* manager);
bool recovery_manager_extension_manual_recover_guided(recovery_manager_extension_t* manager);

// Vérification intégrité extension (utilise persistence existant)
bool recovery_manager_extension_verify_data_integrity_with_existing(recovery_manager_extension_t* manager);
bool recovery_manager_extension_create_emergency_backup_extended(recovery_manager_extension_t* manager);

// Utilitaires extension
recovery_info_extension_t* recovery_info_extension_load(const char* filename);
bool recovery_info_extension_save(const recovery_info_extension_t* info, const char* filename);
void recovery_info_extension_destroy(recovery_info_extension_t* info);

// Initialisation système complète (utilise tous les modules existants)
bool initialize_lum_system_with_auto_recovery_extension(const char* data_directory, 
                                                       const char* wal_filename);

#endif // RECOVERY_MANAGER_EXTENSION_H
