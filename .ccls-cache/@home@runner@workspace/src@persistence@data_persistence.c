#include "data_persistence.h"
#include "storage_backend.h"
#include "../lum/lum_core.h"
#include "../debug/forensic_logger.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

static uint64_t next_transaction_id = 1;

// Context management
persistence_context_t* persistence_context_create(const char* storage_directory) {
    if (!storage_directory) return NULL;

    persistence_context_t* ctx = TRACKED_MALLOC(sizeof(persistence_context_t));
    if (!ctx) return NULL;

    ctx->magic_number = PERSISTENCE_CONTEXT_MAGIC;

    // PRODUCTION: Utilisation paths absolus avec /data/ si disponible
    char absolute_path[MAX_STORAGE_PATH_LENGTH];
    if (strncmp(storage_directory, "/", 1) == 0) {
        // Déjà un path absolu
        strncpy(absolute_path, storage_directory, MAX_STORAGE_PATH_LENGTH - 1);
    } else {
        // Convertir en path absolu pour production
        if (access("/data", F_OK) == 0) {
            snprintf(absolute_path, MAX_STORAGE_PATH_LENGTH, "/data/%s", storage_directory);
        } else {
            // Fallback pour développement - utiliser répertoire courant absolu
            char* cwd = getcwd(NULL, 0);
            if (cwd) {
                snprintf(absolute_path, MAX_STORAGE_PATH_LENGTH, "%s/%s", cwd, storage_directory);
                free(cwd);
            } else {
                strncpy(absolute_path, storage_directory, MAX_STORAGE_PATH_LENGTH - 1);
            }
        }
    }

    strncpy(ctx->storage_directory, absolute_path, MAX_STORAGE_PATH_LENGTH - 1);
    ctx->storage_directory[MAX_STORAGE_PATH_LENGTH - 1] = '\0';

    fprintf(stderr, "[PERSISTENCE] Répertoire configuré: %s\n", ctx->storage_directory);

    ctx->default_format = STORAGE_FORMAT_BINARY;
    ctx->auto_backup_enabled = true;
    ctx->max_backup_count = 5;
    ctx->compression_enabled = false;
    ctx->integrity_checking_enabled = true;
    ctx->transaction_log = NULL;

    // CORRECTION CRITIQUE: Ensure storage directory exists - utiliser le path absolu résolu
    persistence_ensure_directory_exists(ctx->storage_directory);

    // VÉRIFICATION: Test d'écriture pour détecter problèmes déploiement
    char test_file[MAX_STORAGE_PATH_LENGTH];
    
    // Vérification longueur avant snprintf pour éviter troncation
    size_t dir_len = strlen(ctx->storage_directory);
    if (dir_len + 12 >= MAX_STORAGE_PATH_LENGTH) { // 12 = strlen("/.write_test") + 1
        unified_forensic_log(FORENSIC_LEVEL_ERROR, "persistence_context_create",
                           "Storage directory path too long: %zu chars", dir_len);
        TRACKED_FREE(ctx);
        return NULL;
    }
    
    snprintf(test_file, sizeof(test_file), "%s/.write_test", ctx->storage_directory);
    FILE* test_fp = fopen(test_file, "w");
    if (!test_fp) {
        fprintf(stderr, "[PERSISTENCE] ERREUR: Répertoire non accessible en écriture: %s\n", ctx->storage_directory);
        TRACKED_FREE(ctx);
        return NULL;
    }
    fclose(test_fp);
    unlink(test_file); // Nettoyer le fichier test

    return ctx;
}

void persistence_context_destroy(persistence_context_t* ctx) {
    if (!ctx) return;

    if (ctx->transaction_log) {
        fclose(ctx->transaction_log);
    }

    TRACKED_FREE(ctx);
}

bool persistence_context_configure(persistence_context_t* ctx,
                                  storage_format_e format,
                                  bool auto_backup,
                                  size_t max_backups,
                                  bool compression) {
    if (!ctx) return false;

    ctx->default_format = format;
    ctx->auto_backup_enabled = auto_backup;
    ctx->max_backup_count = max_backups;
    ctx->compression_enabled = compression;

    return true;
}

// Storage result management
storage_result_t* storage_result_create(void) {
    storage_result_t* result = TRACKED_MALLOC(sizeof(storage_result_t));
    if (!result) return NULL;

    result->success = false;
    result->filename[0] = '\0';
    result->bytes_written = 0;
    result->bytes_read = 0;
    result->checksum = 0;
    result->error_message[0] = '\0';

    return result;
}

void storage_result_destroy(storage_result_t* result) {
    if (result) {
        TRACKED_FREE(result);
    }
}

void storage_result_set_error(storage_result_t* result, const char* error_message) {
    if (result && error_message) {
        result->success = false;
        strncpy(result->error_message, error_message, sizeof(result->error_message) - 1);
        result->error_message[sizeof(result->error_message) - 1] = '\0';
    }
}

void storage_result_set_success(storage_result_t* result, const char* filename,
                               size_t bytes_processed, uint32_t checksum) {
    if (result) {
        result->success = true;
        if (filename) {
            strncpy(result->filename, filename, sizeof(result->filename) - 1);
            result->filename[sizeof(result->filename) - 1] = '\0';
        }
        result->bytes_written = bytes_processed;
        result->checksum = checksum;
    }
}

// Test compatibility functions implementation
storage_backend_t* storage_backend_create(const char* database_path) {
    if (!database_path) return NULL;

    storage_backend_t* backend = TRACKED_MALLOC(sizeof(storage_backend_t));
    if (!backend) return NULL;

    backend->magic_number = STORAGE_MAGIC_NUMBER;
    strncpy(backend->database_path, database_path, MAX_STORAGE_PATH_LENGTH - 1);
    backend->database_path[MAX_STORAGE_PATH_LENGTH - 1] = '\0';
    backend->current_file[0] = '\0';
    backend->transaction_active = false;

    backend->ctx = persistence_context_create(database_path);
    backend->is_initialized = (backend->ctx != NULL);

    return backend;
}

void storage_backend_destroy(storage_backend_t* backend) {
    if (backend) {
        if (backend->ctx) {
            persistence_context_destroy(backend->ctx);
        }
        TRACKED_FREE(backend);
    }
}

serialized_data_t* serialize_lum(const lum_t* lum) {
    if (!lum) return NULL;

    serialized_data_t* data = TRACKED_MALLOC(sizeof(serialized_data_t));
    if (!data) return NULL;

    data->size = sizeof(lum_t);
    data->data = TRACKED_MALLOC(data->size);
    if (!data->data) {
        TRACKED_FREE(data);
        return NULL;
    }

    memcpy(data->data, lum, data->size);
    return data;
}

lum_t* deserialize_lum(const serialized_data_t* data) {
    if (!data || !data->data || data->size < sizeof(lum_t)) return NULL;

    lum_t* lum = TRACKED_MALLOC(sizeof(lum_t));
    if (!lum) return NULL;

    memcpy(lum, data->data, sizeof(lum_t));
    return lum;
}

void serialized_data_destroy(serialized_data_t* data) {
    if (data) {
        if (data->data) {
            TRACKED_FREE(data->data);
        }
        TRACKED_FREE(data);
    }
}

bool store_lum(storage_backend_t* backend, const char* key, const lum_t* lum) {
    if (!backend || !key || !lum || !backend->is_initialized) return false;

    storage_result_t* result = persistence_save_lum(backend->ctx, lum, key);
    bool success = (result && result->success);

    if (result) {
        storage_result_destroy(result);
    }

    return success;
}

lum_t* load_lum(storage_backend_t* backend, const char* key) {
    if (!backend || !key || !backend->is_initialized) return NULL;

    lum_t* lum = NULL;
    storage_result_t* result = persistence_load_lum(backend->ctx, key, &lum);

    if (result) {
        storage_result_destroy(result);
    }

    return lum;
}

transaction_t* begin_transaction(storage_backend_t* backend) {
    if (!backend || !backend->is_initialized) return NULL;

    transaction_t* transaction = TRACKED_MALLOC(sizeof(transaction_t));
    if (!transaction) return NULL;

    transaction->id = next_transaction_id++;
    SAFE_STRCPY(transaction->operation, "batch_store", sizeof(transaction->operation));
    transaction->timestamp = (uint64_t)time(NULL);
    transaction->committed = false;

    return transaction;
}

bool commit_transaction(transaction_t* transaction) {
    if (!transaction) return false;

    transaction->committed = true;
    TRACKED_FREE(transaction);
    return true;
}

bool storage_backend_store_batch(storage_backend_t* backend, void** objects, size_t count) {
    if (!backend || !objects || count == 0 || !backend->is_initialized) return false;

    // Simple implementation: store each object individually
    for (size_t i = 0; i < count; i++) {
        if (!objects[i]) continue;

        char key[64];
        snprintf(key, sizeof(key), "batch_item_%zu", i);

        // Assume objects are lum_t for simplicity
        if (!store_lum(backend, key, (lum_t*)objects[i])) {
            return false;
        }
    }

    return true;
}


// LUM storage operations
storage_result_t* persistence_save_lum(persistence_context_t* ctx,
                                      const lum_t* lum,
                                      const char* filename) {
    storage_result_t* result = storage_result_create();
    if (!result) return NULL;

    if (!ctx || !lum || !filename) {
        storage_result_set_error(result, "Invalid input parameters");
        return result;
    }

    // SÉCURITÉ: Sanitization du nom de fichier pour éviter path traversal
    if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\')) {
        storage_result_set_error(result, "Nom fichier non securise rejete");
        return result;
    }

    char full_path[MAX_STORAGE_PATH_LENGTH];
    int path_result = snprintf(full_path, sizeof(full_path), "%s/%s", 
                              ctx->storage_directory, filename);

    if (path_result >= (int)sizeof(full_path) || path_result < 0) {
        storage_result_set_error(result, "Chemin fichier trop long");
        return result;
    }


    FILE* file = fopen(full_path, "wb");
    if (!file) {
        storage_result_set_error(result, "Failed to open file for writing");
        return result;
    }

    // Write header
    storage_header_t header = {0};
    header.magic_number = STORAGE_MAGIC_NUMBER;
    header.version = STORAGE_FORMAT_VERSION;
    header.format_type = ctx->default_format;
    header.timestamp = time(NULL);
    header.data_size = sizeof(lum_t);

    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to write header");
        return result;
    }

    // Write LUM data
    if (fwrite(lum, sizeof(lum_t), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to write LUM data");
        return result;
    }

    fclose(file);

    // Calculate checksum
    uint32_t checksum = persistence_calculate_checksum(lum, sizeof(lum_t));

    // Update header with checksum
    file = fopen(full_path, "r+b");
    if (file) {
        fseek(file, offsetof(storage_header_t, checksum), SEEK_SET);
        fwrite(&checksum, sizeof(checksum), 1, file);
        fclose(file);
    }

    storage_result_set_success(result, full_path, sizeof(storage_header_t) + sizeof(lum_t), checksum);

    // Log transaction
    persistence_log_transaction(ctx, "SAVE_LUM", filename, true, sizeof(lum_t));

    return result;
}

storage_result_t* persistence_load_lum(persistence_context_t* ctx,
                                      const char* filename,
                                      lum_t** lum) {
    storage_result_t* result = storage_result_create();
    if (!result) return NULL;

    if (!ctx || !filename || !lum) {
        storage_result_set_error(result, "Invalid input parameters");
        return result;
    }

    // SÉCURITÉ: Sanitization du nom de fichier pour éviter path traversal
    if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\')) {
        storage_result_set_error(result, "Nom fichier non sécurisé rejeté");
        return result;
    }

    char full_path[MAX_STORAGE_PATH_LENGTH];
    int path_result = snprintf(full_path, sizeof(full_path), "%s/%s", 
                              ctx->storage_directory, filename);

    if (path_result >= (int)sizeof(full_path) || path_result < 0) {
        storage_result_set_error(result, "Chemin fichier trop long");
        return result;
    }


    FILE* file = fopen(full_path, "rb");
    if (!file) {
        storage_result_set_error(result, "Failed to open file for reading");
        return result;
    }

    // Read and validate header
    storage_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to read header");
        return result;
    }

    if (header.magic_number != STORAGE_MAGIC_NUMBER) {
        fclose(file);
        storage_result_set_error(result, "Invalid file format");
        return result;
    }

    if (header.version != STORAGE_FORMAT_VERSION) {
        fclose(file);
        storage_result_set_error(result, "Unsupported file version");
        return result;
    }

    // Allocate LUM
    *lum = TRACKED_MALLOC(sizeof(lum_t));
    if (!*lum) {
        fclose(file);
        storage_result_set_error(result, "Memory allocation failed");
        return result;
    }

    // Read LUM data
    if (fread(*lum, sizeof(lum_t), 1, file) != 1) {
        fclose(file);
        TRACKED_FREE(*lum);
        *lum = NULL;
        storage_result_set_error(result, "Failed to read LUM data");
        return result;
    }

    fclose(file);

    // Verify integrity if enabled
    if (ctx->integrity_checking_enabled) {
        uint32_t calculated_checksum = persistence_calculate_checksum(*lum, sizeof(lum_t));
        if (calculated_checksum != header.checksum) {
            TRACKED_FREE(*lum);
            *lum = NULL;
            storage_result_set_error(result, "Data integrity check failed");
            return result;
        }
    }

    storage_result_set_success(result, full_path, sizeof(lum_t), header.checksum);
    result->bytes_read = sizeof(lum_t);

    // Log transaction
    persistence_log_transaction(ctx, "LOAD_LUM", filename, true, sizeof(lum_t));

    return result;
}

// Group storage operations
storage_result_t* persistence_save_group(persistence_context_t* ctx,
                                        const lum_group_t* group,
                                        const char* filename) {
    storage_result_t* result = storage_result_create();
    if (!result) return NULL;

    if (!ctx || !group || !filename) {
        storage_result_set_error(result, "Invalid input parameters");
        return result;
    }

    // SÉCURITÉ: Sanitization du nom de fichier pour éviter path traversal
    if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\')) {
        storage_result_set_error(result, "Nom fichier non sécurisé rejeté");
        return result;
    }

    char full_path[MAX_STORAGE_PATH_LENGTH];
    int path_result = snprintf(full_path, sizeof(full_path), "%s/%s", 
                              ctx->storage_directory, filename);

    if (path_result >= (int)sizeof(full_path) || path_result < 0) {
        storage_result_set_error(result, "Chemin fichier trop long");
        return result;
    }


    FILE* file = fopen(full_path, "wb");
    if (!file) {
        storage_result_set_error(result, "Failed to open file for writing");
        return result;
    }

    // Calculate total data size
    size_t group_header_size = sizeof(uint32_t) * 3; // group_id, count, capacity
    size_t lums_data_size = group->count * sizeof(lum_t);
    size_t total_data_size = group_header_size + lums_data_size;

    // Write storage header
    storage_header_t header = {0};
    header.magic_number = STORAGE_MAGIC_NUMBER;
    header.version = STORAGE_FORMAT_VERSION;
    header.format_type = ctx->default_format;
    header.timestamp = time(NULL);
    header.data_size = total_data_size;
    SAFE_STRCPY(header.metadata, "LUM_GROUP", sizeof(header.metadata));

    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to write header");
        return result;
    }

    // Write group metadata
    if (fwrite(&group->group_id, sizeof(group->group_id), 1, file) != 1 ||
        fwrite(&group->count, sizeof(group->count), 1, file) != 1 ||
        fwrite(&group->capacity, sizeof(group->capacity), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to write group metadata");
        return result;
    }

    // Write LUMs data
    if (group->count > 0 && group->lums) {
        if (fwrite(group->lums, sizeof(lum_t), group->count, file) != group->count) {
            fclose(file);
            storage_result_set_error(result, "Failed to write LUMs data");
            return result;
        }
    }

    fclose(file);

    // Calculate checksum
    uint32_t checksum = persistence_calculate_checksum(&group->group_id, sizeof(group->group_id));
    if (group->lums && group->count > 0) {
        checksum ^= persistence_calculate_checksum(group->lums, lums_data_size);
    }

    // Update header with checksum
    file = fopen(full_path, "r+b");
    if (file) {
        fseek(file, offsetof(storage_header_t, checksum), SEEK_SET);
        fwrite(&checksum, sizeof(checksum), 1, file);
        fclose(file);
    }

    storage_result_set_success(result, full_path, sizeof(storage_header_t) + total_data_size, checksum);

    // Log transaction
    persistence_log_transaction(ctx, "SAVE_GROUP", filename, true, total_data_size);

    return result;
}

storage_result_t* persistence_load_group(persistence_context_t* ctx,
                                        const char* filename,
                                        lum_group_t** group) {
    storage_result_t* result = storage_result_create();
    if (!result) return NULL;

    if (!ctx || !filename || !group) {
        storage_result_set_error(result, "Invalid input parameters");
        return result;
    }

    // SÉCURITÉ: Sanitization du nom de fichier pour éviter path traversal
    if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\')) {
        storage_result_set_error(result, "Nom fichier non sécurisé rejeté");
        return result;
    }

    char full_path[MAX_STORAGE_PATH_LENGTH];
    int path_result = snprintf(full_path, sizeof(full_path), "%s/%s", 
                              ctx->storage_directory, filename);

    if (path_result >= (int)sizeof(full_path) || path_result < 0) {
        storage_result_set_error(result, "Chemin fichier trop long");
        return result;
    }


    FILE* file = fopen(full_path, "rb");
    if (!file) {
        storage_result_set_error(result, "Failed to open file for reading");
        return result;
    }

    // Read and validate header
    storage_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to read header");
        return result;
    }

    if (header.magic_number != STORAGE_MAGIC_NUMBER) {
        fclose(file);
        storage_result_set_error(result, "Invalid file format");
        return result;
    }

    if (header.version != STORAGE_FORMAT_VERSION) {
        fclose(file);
        storage_result_set_error(result, "Unsupported file version");
        return result;
    }

    // Read group metadata
    uint32_t group_id, count, capacity;
    if (fread(&group_id, sizeof(group_id), 1, file) != 1 ||
        fread(&count, sizeof(count), 1, file) != 1 ||
        fread(&capacity, sizeof(capacity), 1, file) != 1) {
        fclose(file);
        storage_result_set_error(result, "Failed to read group metadata");
        return result;
    }

    // Create group
    *group = lum_group_create(capacity);
    if (!*group) {
        fclose(file);
        storage_result_set_error(result, "Failed to create group");
        return result;
    }

    (*group)->group_id = group_id;
    (*group)->count = 0; // Will be incremented as we add LUMs

    // Read LUMs data
    for (uint32_t i = 0; i < count; i++) {
        lum_t temp_lum;
        if (fread(&temp_lum, sizeof(lum_t), 1, file) != 1) {
            fclose(file);
            lum_group_destroy(*group);
            *group = NULL;
            storage_result_set_error(result, "Failed to read LUM data");
            return result;
        }

        // Add LUM to group (this will increment count)
        if (!lum_group_add(*group, &temp_lum)) {
            fclose(file);
            lum_group_destroy(*group);
            *group = NULL;
            storage_result_set_error(result, "Failed to add LUM to group");
            return result;
        }
    }

    fclose(file);

    // Verify integrity if enabled
    if (ctx->integrity_checking_enabled) {
        uint32_t calculated_checksum = persistence_calculate_checksum(&(*group)->group_id, sizeof((*group)->group_id));
        if ((*group)->lums && (*group)->count > 0) {
            calculated_checksum ^= persistence_calculate_checksum((*group)->lums, (*group)->count * sizeof(lum_t));
        }
        if (calculated_checksum != header.checksum) {
            lum_group_destroy(*group);
            *group = NULL;
            storage_result_set_error(result, "Data integrity check failed");
            return result;
        }
    }

    storage_result_set_success(result, full_path, header.data_size, header.checksum);

    // Log transaction
    persistence_log_transaction(ctx, "LOAD_GROUP", filename, true, header.data_size);

    return result;
}

// Utility functions
bool persistence_ensure_directory_exists(const char* directory) {
    if (!directory) return false;

    struct stat st = {0};
    if (stat(directory, &st) == -1) {
        return mkdir(directory, 0755) == 0;
    }

    return S_ISDIR(st.st_mode);
}

bool persistence_file_exists(const char* filename) {
    if (!filename) return false;

    struct stat st;
    return stat(filename, &st) == 0;
}

size_t persistence_get_file_size(const char* filename) {
    if (!filename) return 0;

    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }

    return 0;
}

uint32_t persistence_calculate_checksum(const void* data, size_t size) {
    if (!data || size == 0) return 0;

    uint32_t checksum = 0;
    const uint8_t* bytes = (const uint8_t*)data;

    for (size_t i = 0; i < size; i++) {
        checksum ^= bytes[i];
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
    }

    return checksum;
}

// Transaction logging
bool persistence_start_transaction_log(persistence_context_t* ctx) {
    if (!ctx) return false;

    char log_path[MAX_STORAGE_PATH_LENGTH];

    // Vérification taille avant snprintf pour éviter truncation
    size_t dir_len = strlen(ctx->storage_directory);
    if (dir_len + 18 >= MAX_STORAGE_PATH_LENGTH) { // 18 = strlen("/transactions.log") + 1
        unified_forensic_log(FORENSIC_LEVEL_ERROR, "persistence_start_transaction_log",
                           "Storage directory path too long: %zu chars", dir_len);
        return false;
    }

    snprintf(log_path, sizeof(log_path), "%s/transactions.log", ctx->storage_directory);


    ctx->transaction_log = fopen(log_path, "a");
    return ctx->transaction_log != NULL;
}

bool persistence_stop_transaction_log(persistence_context_t* ctx) {
    if (!ctx || !ctx->transaction_log) return false;

    fclose(ctx->transaction_log);
    ctx->transaction_log = NULL;

    return true;
}

bool persistence_log_transaction(persistence_context_t* ctx,
                                const char* operation,
                                const char* filename,
                                bool success,
                                size_t data_size) {
    if (!ctx || !operation || !filename) return false;

    // Create log entry even if transaction log is not active
    transaction_record_t record;
    record.transaction_id = next_transaction_id++;
    strncpy(record.operation, operation, sizeof(record.operation) - 1);
    record.operation[sizeof(record.operation) - 1] = '\0';
    strncpy(record.filename, filename, sizeof(record.filename) - 1);
    record.filename[sizeof(record.filename) - 1] = '\0';
    record.timestamp = time(NULL);
    record.success = success;
    record.data_size = data_size;

    if (ctx->transaction_log) {
        return fwrite(&record, sizeof(record), 1, ctx->transaction_log) == 1;
    }

    return true; // Success even without active log
}

// Serialization helpers
size_t persistence_serialize_lum(const lum_t* lum, uint8_t* buffer, size_t buffer_size) {
    if (!lum || !buffer || buffer_size < sizeof(lum_t)) return 0;

    memcpy(buffer, lum, sizeof(lum_t));
    return sizeof(lum_t);
}

size_t persistence_deserialize_lum(const uint8_t* buffer, size_t buffer_size, lum_t* lum) {
    if (!buffer || !lum || buffer_size < sizeof(lum_t)) return 0;

    memcpy(lum, buffer, sizeof(lum_t));
    return sizeof(lum_t);
}

// Additions pour la récupération et la vérification
bool persistence_verify_file_integrity(persistence_context_t* ctx, const char* filepath) {
    if (!ctx || ctx->magic_number != PERSISTENCE_CONTEXT_MAGIC) return false;
    if (!filepath) return false;

    forensic_log(FORENSIC_LEVEL_DEBUG, "persistence_verify_file_integrity",
                "Verification integrite fichier: %s", filepath);

    // Vérifier existence du fichier
    struct stat file_stat;
    if (stat(filepath, &file_stat) != 0) {
        forensic_log(FORENSIC_LEVEL_ERROR, "persistence_verify_file_integrity",
                    "Fichier inexistant: %s", filepath);
        return false;
    }

    // Vérifier taille minimale
    if (file_stat.st_size < 16) {
        forensic_log(FORENSIC_LEVEL_ERROR, "persistence_verify_file_integrity",
                    "Fichier trop petit: %s (taille: %ld)", filepath, file_stat.st_size);
        return false;
    }

    // Ouvrir et vérifier magic number
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        forensic_log(FORENSIC_LEVEL_ERROR, "persistence_verify_file_integrity",
                    "Impossible ouvrir fichier: %s", filepath);
        return false;
    }

    uint32_t file_magic;
    size_t read_bytes = fread(&file_magic, sizeof(uint32_t), 1, file);
    fclose(file);

    if (read_bytes != 1) {
        forensic_log(FORENSIC_LEVEL_ERROR, "persistence_verify_file_integrity",
                    "Impossible lire magic number: %s", filepath);
        return false;
    }

    // Vérifier magic numbers valides
    bool valid_magic = (file_magic == PERSISTENCE_CONTEXT_MAGIC ||
                       file_magic == LUM_MAGIC_NUMBER ||
                       file_magic == LUM_GROUP_MAGIC ||
                       file_magic == 0x52454356); // RECOVERY_MANAGER_MAGIC

    if (!valid_magic) {
        forensic_log(FORENSIC_LEVEL_ERROR, "persistence_verify_file_integrity",
                    "Magic number invalide: %s (0x%08X)", filepath, file_magic);
        return false;
    }

    forensic_log(FORENSIC_LEVEL_DEBUG, "persistence_verify_file_integrity",
                "Fichier integre: %s (magic: 0x%08X)", filepath, file_magic);
    return true;
}

// Note: wal_extension_replay_from_existing_persistence and wal_extension_create_checkpoint_with_existing
// are now implemented in transaction_wal_extension.c

// Function to verify file integrity for recovery manager
bool recovery_manager_verify_file_integrity(persistence_context_t* ctx, const char* filepath) {
    // This function seems to be a duplicate or a specific version for the recovery manager.
    // We can call the generic persistence_verify_file_integrity or implement specific logic if needed.
    // For now, let's assume it calls the generic one.
    forensic_log(FORENSIC_LEVEL_INFO, "recovery_manager_verify_file_integrity", "Called with %s", filepath);
    return persistence_verify_file_integrity(ctx, filepath);
}