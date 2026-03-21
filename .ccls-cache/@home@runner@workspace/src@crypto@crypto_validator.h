
#ifndef CRYPTO_VALIDATOR_H
#define CRYPTO_VALIDATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "../lum/lum_core.h"

#define SHA256_DIGEST_SIZE 32
#define SHA256_BLOCK_SIZE 64
#define MAX_HASH_STRING_LENGTH 65

// SHA-256 context structure
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[SHA256_BLOCK_SIZE];
    size_t buffer_length;
} sha256_context_t;

// Validation result structure
typedef struct {
    bool is_valid;
    char computed_hash[MAX_HASH_STRING_LENGTH];
    char expected_hash[MAX_HASH_STRING_LENGTH];
    size_t data_size;
    char error_message[256];
} crypto_validation_result_t;

// File integrity structure
typedef struct {
    char filename[256];
    char hash[MAX_HASH_STRING_LENGTH];
    size_t file_size;
    time_t last_modified;
    bool is_verified;
} file_integrity_t;

// Integrity database
typedef struct {
    file_integrity_t* files;
    size_t file_count;
    size_t capacity;
    char database_path[512];
    bool is_initialized;
} integrity_database_t;

// Function declarations

// SHA-256 implementation
void sha256_init(sha256_context_t* ctx);
void sha256_update(sha256_context_t* ctx, const uint8_t* data, size_t len);
void sha256_final(sha256_context_t* ctx, uint8_t* digest);
void sha256_hash(const uint8_t* data, size_t len, uint8_t* digest);
void sha256_process_block(sha256_context_t* ctx, const uint8_t* block);

// Hash utilities
void bytes_to_hex_string(const uint8_t* bytes, size_t len, char* hex_string);
bool hex_string_to_bytes(const char* hex_string, uint8_t* bytes, size_t max_len);
bool compute_file_hash(const char* filename, char* hash_string);
bool compute_data_hash(const void* data, size_t size, char* hash_string);

// Additional crypto types and enums
typedef enum {
    CRYPTO_ALGO_SHA256
} crypto_algorithm_e;

typedef struct {
    crypto_algorithm_e algorithm;
    bool is_initialized;
    size_t total_operations;
    time_t last_operation_time;
} hash_calculator_t;

typedef struct {
    uint8_t signature_data[64];
    size_t signature_length;
    time_t timestamp;
    bool is_valid;
} signature_result_t;

// LUM compatibility constant
#define LUM_STRUCTURE_MAX 4

// Validation functions
crypto_validation_result_t* crypto_validate_data(const void* data, size_t size, 
                                                const char* expected_hash);
crypto_validation_result_t* crypto_validate_file(const char* filename, 
                                                 const char* expected_hash);
void crypto_validation_result_destroy(crypto_validation_result_t* result);
bool crypto_validate_sha256_implementation(void);

// Integrity database management
integrity_database_t* integrity_database_create(const char* database_path);
void integrity_database_destroy(integrity_database_t* db);
bool integrity_database_add_file(integrity_database_t* db, const char* filename);
bool integrity_database_verify_file(integrity_database_t* db, const char* filename);
bool integrity_database_verify_all(integrity_database_t* db);
bool integrity_database_save(integrity_database_t* db);
bool integrity_database_load(integrity_database_t* db);

// Batch validation
bool crypto_validate_directory(const char* directory_path, const char* checksums_file);
bool crypto_generate_checksums(const char* directory_path, const char* output_file);

// LUM/VORAX specific validation
bool crypto_validate_lum_data(const void* lum_data, size_t size);
bool crypto_validate_execution_log(const char* log_file);
bool crypto_validate_source_files(void);

// Chain of custody
typedef struct {
    char operation[128];
    char hash_before[MAX_HASH_STRING_LENGTH];
    char hash_after[MAX_HASH_STRING_LENGTH];
    time_t timestamp;
    char metadata[256];
} custody_record_t;

typedef struct {
    custody_record_t* records;
    size_t record_count;
    size_t capacity;
    bool is_sealed;
} custody_chain_t;

custody_chain_t* custody_chain_create(void);
void custody_chain_destroy(custody_chain_t* chain);
bool custody_chain_add_record(custody_chain_t* chain, const char* operation,
                             const char* hash_before, const char* hash_after,
                             const char* metadata);
bool custody_chain_verify(custody_chain_t* chain);
bool custody_chain_export(custody_chain_t* chain, const char* filename);

// Hash calculator functions
hash_calculator_t* hash_calculator_create(void);
void hash_calculator_destroy(hash_calculator_t* calc);
bool compute_data_hash(const void* data, size_t size, char* hash_string);
bool verify_data_integrity(const void* data, size_t data_size, const char* expected_hash);
bool validate_lum_integrity(const lum_t* lum);

// Digital signature functions
signature_result_t* generate_digital_signature(const void* data, size_t data_size);
bool verify_digital_signature(const void* data, size_t data_size, const signature_result_t* signature);
void signature_result_destroy(signature_result_t* result);

// Internal helper functions
int hex_char_to_int(char c);
uint32_t rotr(uint32_t n, uint32_t x);
uint32_t choose(uint32_t e, uint32_t f, uint32_t g);
uint32_t majority(uint32_t a, uint32_t b, uint32_t c);
uint32_t sig0(uint32_t x);
uint32_t sig1(uint32_t x);
void sha256_transform(sha256_context_t* ctx);

#endif // CRYPTO_VALIDATOR_H
