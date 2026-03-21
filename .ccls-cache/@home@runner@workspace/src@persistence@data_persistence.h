#ifndef DATA_PERSISTENCE_H
#define DATA_PERSISTENCE_H

#include "../lum/lum_core.h"
#include "../common/common_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_FILENAME_LENGTH 256
#define STORAGE_FORMAT_VERSION 1
#define STORAGE_MAGIC_NUMBER 0x4C554D58 // "LUMX"
#define PERSISTENCE_CONTEXT_MAGIC 0xDEADBEEF
#define LUM_MAGIC_NUMBER 0x4C554D00
// LUM_GROUP_MAGIC moved to src/common/magic_numbers.h for consistency

// Storage format types
typedef enum {
    STORAGE_FORMAT_BINARY,
    STORAGE_FORMAT_JSON,
    STORAGE_FORMAT_COMPRESSED
} storage_format_e;

// Storage header
typedef struct {
    uint32_t magic_number;
    uint16_t version;
    uint16_t format_type;
    uint64_t timestamp;
    uint64_t data_size;
    uint32_t checksum;
    char metadata[128];
} storage_header_t;

// Persistence context
typedef struct {
    uint32_t magic_number;
    char storage_directory[MAX_STORAGE_PATH_LENGTH];
    storage_format_e default_format;
    bool auto_backup_enabled;
    size_t max_backup_count;
    bool compression_enabled;
    bool integrity_checking_enabled;
    FILE* transaction_log;
} persistence_context_t;

// Storage result
typedef struct {
    bool success;
    char filename[MAX_STORAGE_PATH_LENGTH];
    size_t bytes_written;
    size_t bytes_read;
    uint32_t checksum;
    char error_message[256];
    void* transaction_ref;
} storage_result_t;

// Transaction operation types
typedef enum {
    TRANSACTION_BEGIN,
    TRANSACTION_COMMIT,
    TRANSACTION_ROLLBACK,
    TRANSACTION_WRITE,
    TRANSACTION_READ
} transaction_operation_type_e;

// Transaction record
typedef struct {
    uint64_t transaction_id;
    transaction_operation_type_e operation_type;
    char operation[64];
    char filename[MAX_STORAGE_PATH_LENGTH];
    uint64_t timestamp;
    bool success;
    size_t data_size;
    uint64_t lum_id;
} transaction_record_t;

// Function declarations

// Context management
persistence_context_t* persistence_context_create(const char* storage_directory);
void persistence_context_destroy(persistence_context_t* ctx);
bool persistence_context_configure(persistence_context_t* ctx,
                                  storage_format_e format,
                                  bool auto_backup,
                                  size_t max_backups,
                                  bool compression);

// LUM storage operations
storage_result_t* persistence_save_lum(persistence_context_t* ctx,
                                      const lum_t* lum,
                                      const char* filename);
storage_result_t* persistence_load_lum(persistence_context_t* ctx,
                                      const char* filename,
                                      lum_t** lum);

// Group storage operations
storage_result_t* persistence_save_group(persistence_context_t* ctx,
                                        const lum_group_t* group,
                                        const char* filename);
storage_result_t* persistence_load_group(persistence_context_t* ctx,
                                        const char* filename,
                                        lum_group_t** group);

// Zone storage operations
storage_result_t* persistence_save_zone(persistence_context_t* ctx,
                                       const lum_zone_t* zone,
                                       const char* filename);
storage_result_t* persistence_load_zone(persistence_context_t* ctx,
                                       const char* filename,
                                       lum_zone_t** zone);

// Batch operations
bool persistence_save_all_zones(persistence_context_t* ctx,
                               lum_zone_t** zones,
                               size_t zone_count,
                               const char* batch_name);
bool persistence_load_all_zones(persistence_context_t* ctx,
                               const char* batch_name,
                               lum_zone_t*** zones,
                               size_t* zone_count);

// Backup management
bool persistence_create_backup(persistence_context_t* ctx,
                              const char* source_file,
                              const char* backup_name);
bool persistence_restore_backup(persistence_context_t* ctx,
                               const char* backup_name,
                               const char* target_file);
bool persistence_list_backups(persistence_context_t* ctx,
                             const char* base_filename,
                             char*** backup_list,
                             size_t* backup_count);
bool persistence_cleanup_old_backups(persistence_context_t* ctx);

// Format conversion
storage_result_t* persistence_convert_format(persistence_context_t* ctx,
                                            const char* input_file,
                                            const char* output_file,
                                            storage_format_e target_format);

// Transaction logging
bool persistence_start_transaction_log(persistence_context_t* ctx);
bool persistence_stop_transaction_log(persistence_context_t* ctx);
bool persistence_log_transaction(persistence_context_t* ctx,
                                const char* operation,
                                const char* filename,
                                bool success,
                                size_t data_size);
bool persistence_replay_transactions(persistence_context_t* ctx,
                                    const char* log_file);

// Data integrity
bool persistence_verify_file_integrity(persistence_context_t* ctx,
                                      const char* filename);
bool persistence_repair_corrupted_file(persistence_context_t* ctx,
                                      const char* filename);
uint32_t persistence_calculate_checksum(const void* data, size_t size);

// Storage utilities
bool persistence_ensure_directory_exists(const char* directory);
bool persistence_file_exists(const char* filename);
size_t persistence_get_file_size(const char* filename);
bool persistence_delete_file(const char* filename);
bool persistence_copy_file(const char* source, const char* destination);

// Serialization helpers
size_t persistence_serialize_lum(const lum_t* lum, uint8_t* buffer, size_t buffer_size);
size_t persistence_deserialize_lum(const uint8_t* buffer, size_t buffer_size, lum_t* lum);
size_t persistence_serialize_group(const lum_group_t* group, uint8_t* buffer, size_t buffer_size);
size_t persistence_deserialize_group(const uint8_t* buffer, size_t buffer_size, lum_group_t** group);

// JSON format support
bool persistence_lum_to_json(const lum_t* lum, char* json_buffer, size_t buffer_size);
bool persistence_json_to_lum(const char* json_string, lum_t* lum);
bool persistence_group_to_json(const lum_group_t* group, char* json_buffer, size_t buffer_size);
bool persistence_json_to_group(const char* json_string, lum_group_t** group);

// Storage result management
storage_result_t* storage_result_create(void);
void storage_result_destroy(storage_result_t* result);
void storage_result_set_error(storage_result_t* result, const char* error_message);
void storage_result_set_success(storage_result_t* result, const char* filename,
                               size_t bytes_processed, uint32_t checksum);

// Forward declaration
typedef struct storage_backend_s storage_backend_t;

// Simple storage backend for testing
struct storage_backend_s {
    uint32_t magic_number;          // Membre ajouté pour validation
    char database_path[MAX_STORAGE_PATH_LENGTH];
    persistence_context_t* ctx;
    bool is_initialized;
    char current_file[256];
    bool transaction_active;
};

typedef struct {
    uint8_t* data;
    size_t size;
} serialized_data_t;

typedef struct {
    uint64_t id;
    uint64_t transaction_id;
    char operation[64];
    uint64_t timestamp;
    bool committed;
} transaction_t;

// Test compatibility functions
storage_backend_t* storage_backend_create(const char* database_path);
void storage_backend_destroy(storage_backend_t* backend);
serialized_data_t* serialize_lum(const lum_t* lum);
lum_t* deserialize_lum(const serialized_data_t* data);
void serialized_data_destroy(serialized_data_t* data);
bool store_lum(storage_backend_t* backend, const char* key, const lum_t* lum);
lum_t* load_lum(storage_backend_t* backend, const char* key);
transaction_t* begin_transaction(storage_backend_t* backend);
bool commit_transaction(transaction_t* transaction);
bool storage_backend_store_batch(storage_backend_t* backend, void** objects, size_t count);

#endif // DATA_PERSISTENCE_H