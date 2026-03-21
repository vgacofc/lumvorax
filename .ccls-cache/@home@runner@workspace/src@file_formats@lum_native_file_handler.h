#ifndef LUM_NATIVE_FILE_HANDLER_H
#define LUM_NATIVE_FILE_HANDLER_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Module Fichier Natif LUM/VORAX Multi-Format
// Conforme prompt.txt - gestion complète formats fichiers

// Types de formats supportés
typedef enum {
    LUM_FORMAT_NATIVE_BINARY = 0,    // Format binaire natif optimisé
    LUM_FORMAT_TEXT_READABLE = 1,    // Format texte lisible humain
    LUM_FORMAT_JSON_STRUCTURED = 2,  // Format JSON structuré
    LUM_FORMAT_XML_STANDARD = 3,     // Format XML avec schéma
    LUM_FORMAT_CSV_TABULAR = 4,      // Format CSV tabulaire
    LUM_FORMAT_COMPRESSED_LZ4 = 5,   // Format compressé LZ4
    LUM_FORMAT_COMPRESSED_ZLIB = 6,  // Format compressé ZLIB
    LUM_FORMAT_VORAX_OPERATIONS = 7, // Format opérations VORAX
    LUM_FORMAT_BINARY_OPTIMIZED = 8, // Format binaire haute performance
    LUM_FORMAT_FORENSIC_AUDIT = 9    // Format audit forensique
} lum_file_format_e;

// Configuration export/import
typedef struct {
    lum_file_format_e format;         // Format cible
    bool include_metadata;            // Inclure métadonnées
    bool include_timestamps;          // Inclure horodatage
    bool include_checksums;           // Inclure checksums
    bool compress_data;               // Compresser données
    bool validate_integrity;         // Valider intégrité
    double compression_level;         // Niveau compression [0,1]
    size_t buffer_size;              // Taille buffer I/O
    char* custom_encoding;           // Encodage personnalisé
    void* memory_address;            // Protection double-free OBLIGATOIRE
    uint32_t config_magic;           // Validation intégrité
} lum_file_config_t;

// Résultat opération fichier
typedef struct {
    bool success;                    // Succès opération
    char filename[512];              // Nom fichier traité
    size_t bytes_processed;          // Octets traités
    size_t lums_processed;           // LUMs traités
    uint64_t processing_time_ns;     // Temps traitement (ns)
    uint32_t checksum;               // Checksum calculé
    char error_message[256];         // Message erreur
    lum_file_format_e format_used;   // Format utilisé
    void* memory_address;            // Protection double-free OBLIGATOIRE
    uint32_t result_magic;           // Validation intégrité
} lum_file_result_t;

// Métadonnées fichier LUM
typedef struct {
    uint32_t magic_number;           // Magic number "LUMV" = 0x4C554D56
    uint16_t version_major;          // Version majeure
    uint16_t version_minor;          // Version mineure
    lum_file_format_e format_type;   // Type de format
    uint64_t creation_timestamp;     // Timestamp création
    uint64_t modification_timestamp; // Timestamp modification
    uint32_t total_lums;             // Nombre total LUMs
    uint32_t total_groups;           // Nombre total groupes
    uint64_t total_size_bytes;       // Taille totale octets
    uint32_t checksum_crc32;         // Checksum CRC32
    char creator_info[64];           // Info créateur
    char file_description[128];      // Description fichier
    void* memory_address;            // Protection double-free OBLIGATOIRE
} lum_file_metadata_t;

// Contexte lecteur/écrivain fichier
typedef struct {
    FILE* file_handle;               // Handle fichier
    lum_file_config_t* config;       // Configuration
    lum_file_metadata_t* metadata;   // Métadonnées
    uint8_t* io_buffer;              // Buffer I/O
    size_t buffer_position;          // Position dans buffer
    size_t buffer_size;              // Taille buffer
    bool is_reading;                 // Mode lecture
    bool is_writing;                 // Mode écriture
    void* memory_address;            // Protection double-free OBLIGATOIRE
    uint32_t context_magic;          // Validation intégrité
} lum_file_context_t;

// Statistiques performance I/O
typedef struct {
    uint64_t files_written;          // Fichiers écrits
    uint64_t files_read;             // Fichiers lus
    uint64_t total_bytes_written;    // Octets écrits total
    uint64_t total_bytes_read;       // Octets lus total
    uint64_t total_lums_exported;    // LUMs exportés total
    uint64_t total_lums_imported;    // LUMs importés total
    double avg_write_speed_mbps;     // Vitesse écriture moyenne (MB/s)
    double avg_read_speed_mbps;      // Vitesse lecture moyenne (MB/s)
    uint64_t compression_ratio;      // Ratio compression [0-100]
    uint64_t total_errors;           // Erreurs totales
    void* memory_address;            // Protection double-free OBLIGATOIRE
} lum_file_stats_t;

// ================== FONCTIONS PRINCIPALES ==================

// Gestion configuration
lum_file_config_t* lum_file_config_create(lum_file_format_e format);
void lum_file_config_destroy(lum_file_config_t** config_ptr);
bool lum_file_config_set_format(lum_file_config_t* config, lum_file_format_e format);
bool lum_file_config_enable_compression(lum_file_config_t* config, double level);
bool lum_file_config_set_buffer_size(lum_file_config_t* config, size_t size);

// Gestion métadonnées
lum_file_metadata_t* lum_file_metadata_create(void);
void lum_file_metadata_destroy(lum_file_metadata_t** metadata_ptr);
bool lum_file_metadata_update(lum_file_metadata_t* metadata, const lum_group_t* group);
bool lum_file_metadata_validate(const lum_file_metadata_t* metadata);

// Gestion contexte I/O
lum_file_context_t* lum_file_context_create(const char* filename, lum_file_config_t* config, bool writing);
void lum_file_context_destroy(lum_file_context_t** context_ptr);
bool lum_file_context_open(lum_file_context_t* context, const char* filename, const char* mode);
bool lum_file_context_close(lum_file_context_t* context);

// ================== EXPORT MULTI-FORMAT ==================

// Export LUM individuel
lum_file_result_t* lum_export_single(const lum_t* lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_single_binary(const lum_t* lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_single_text(const lum_t* lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_single_json(const lum_t* lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_single_xml(const lum_t* lum, const char* filename, lum_file_config_t* config);

// Export groupe LUM
lum_file_result_t* lum_export_group(const lum_group_t* group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_group_binary(const lum_group_t* group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_group_text(const lum_group_t* group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_group_json(const lum_group_t* group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_group_xml(const lum_group_t* group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_export_group_csv(const lum_group_t* group, const char* filename, lum_file_config_t* config);

// Export multi-groupe
lum_file_result_t* lum_export_multi_groups(lum_group_t** groups, size_t group_count, const char* filename, lum_file_config_t* config);

// ================== IMPORT MULTI-FORMAT ==================

// Import LUM individuel
lum_file_result_t* lum_import_single(lum_t** lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_single_binary(lum_t** lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_single_text(lum_t** lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_single_json(lum_t** lum, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_single_xml(lum_t** lum, const char* filename, lum_file_config_t* config);

// Import groupe LUM
lum_file_result_t* lum_import_group(lum_group_t** group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_group_binary(lum_group_t** group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_group_text(lum_group_t** group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_group_json(lum_group_t** group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_group_xml(lum_group_t** group, const char* filename, lum_file_config_t* config);
lum_file_result_t* lum_import_group_csv(lum_group_t** group, const char* filename, lum_file_config_t* config);

// Import multi-groupe
lum_file_result_t* lum_import_multi_groups(lum_group_t*** groups, size_t* group_count, const char* filename, lum_file_config_t* config);

// ================== UTILITAIRES AVANCÉS ==================

// Détection automatique format
lum_file_format_e lum_file_detect_format(const char* filename);
bool lum_file_validate_format(const char* filename, lum_file_format_e expected_format);
bool lum_file_is_valid_lum_file(const char* filename);

// Conversion formats
lum_file_result_t* lum_file_convert_format(const char* input_filename, const char* output_filename, 
                                         lum_file_format_e source_format, lum_file_format_e target_format, 
                                         lum_file_config_t* config);

// Compression/décompression
lum_file_result_t* lum_file_compress(const char* input_filename, const char* output_filename, 
                                   double compression_level, lum_file_config_t* config);
lum_file_result_t* lum_file_decompress(const char* input_filename, const char* output_filename, 
                                     lum_file_config_t* config);

// Validation intégrité
bool lum_file_verify_integrity(const char* filename, lum_file_config_t* config);
uint32_t lum_file_calculate_checksum(const char* filename);
bool lum_file_repair_corruption(const char* filename, lum_file_config_t* config);

// Statistiques et monitoring
lum_file_stats_t* lum_file_stats_create(void);
void lum_file_stats_destroy(lum_file_stats_t** stats_ptr);
bool lum_file_stats_update(lum_file_stats_t* stats, const lum_file_result_t* result);
lum_file_result_t* lum_file_get_info(const char* filename);

// ================== TESTS STRESS 100M+ ==================

// Tests performance I/O
bool lum_file_stress_test_100m_export(lum_file_config_t* config);
bool lum_file_stress_test_100m_import(lum_file_config_t* config);
bool lum_file_stress_test_all_formats(size_t lum_count, lum_file_config_t* config);
lum_file_result_t* lum_file_benchmark_formats(size_t lum_count, lum_file_config_t* config);

// ================== SÉRIALISATION SÉCURISÉE ==================

// Calcul checksum CRC32
uint32_t lum_calculate_crc32(const uint8_t* data, size_t length);

// Sérialisation LUM sans pointeurs
bool lum_write_serialized(FILE* file, const lum_t* lum);
bool lum_read_serialized(FILE* file, lum_t* lum);

// Sérialisation métadonnées sans pointeurs
bool lum_write_metadata_serialized(FILE* file, const lum_file_metadata_t* metadata);
bool lum_read_metadata_serialized(FILE* file, lum_file_metadata_t* metadata);

// Validation intégrité fichier complet
bool lum_file_verify_integrity_file(const char* filename);

// Test stress I/O 100M+ avec sérialisation sécurisée
bool lum_file_stress_test_100m_write_read(void);

// Tests robustesse
bool lum_file_test_corruption_recovery(lum_file_config_t* config);
bool lum_file_test_large_files(size_t max_size_gb, lum_file_config_t* config);
bool lum_file_test_concurrent_access(size_t thread_count, lum_file_config_t* config);

// ================== RÉSULTATS ET NETTOYAGE ==================

// Gestion résultats
lum_file_result_t* lum_file_result_create(void);
void lum_file_result_destroy(lum_file_result_t** result_ptr);
bool lum_file_result_set_success(lum_file_result_t* result, const char* filename, size_t bytes, size_t lums);
bool lum_file_result_set_error(lum_file_result_t* result, const char* error_message);

// CORRECTION RAPPORT 117: Configuration adaptative
lum_file_config_t* lum_file_config_create_default(void);
lum_file_config_t* lum_file_config_create_adaptive(size_t expected_lum_count);
lum_file_config_t* lum_file_config_create_high_performance(void);
lum_file_config_t* lum_file_config_create_high_compression(void);
lum_file_config_t* lum_file_config_create_forensic_audit(void);

// Configuration messages d'erreur localisables
typedef struct {
    const char* success_message;
    const char* invalid_params_message;
    const char* file_open_error_message;
    const char* memory_error_message;
    const char* format_error_message;
} lum_file_error_messages_t;

lum_file_error_messages_t* lum_file_get_error_messages(void);
bool lum_file_set_custom_error_messages(const lum_file_error_messages_t* messages);

// ================== CONSTANTES DYNAMIQUES ==================

#define LUM_FILE_MAGIC_NUMBER 0x4C554D56      // "LUMV"
#define LUM_FILE_VERSION_MAJOR 1
#define LUM_FILE_VERSION_MINOR 0

// CORRECTION RAPPORT 117: Buffer size dynamique selon système
size_t lum_file_get_optimal_buffer_size(void);
size_t lum_file_calculate_buffer_size_for_count(size_t lum_count);

#define LUM_FILE_MAX_FILENAME_LENGTH 512
#define LUM_FILE_MAX_ERROR_MESSAGE 256
#define LUM_FILE_CONFIG_MAGIC 0xF11E1234      // "FILE"
#define LUM_FILE_RESULT_MAGIC 0xFE51734       // "RESULT" tronqué
#define LUM_FILE_CONTEXT_MAGIC 0xC047EF7      // "CONTEXT" tronqué
#define LUM_FILE_DESTROYED_MAGIC 0xDEADBEEF

// Constantes performance
#define LUM_FILE_MIN_BUFFER_SIZE 1024
#define LUM_FILE_MAX_BUFFER_SIZE (64 * 1024 * 1024)  // 64MB
#define LUM_FILE_DEFAULT_COMPRESSION_LEVEL 0.5
#define LUM_FILE_MIN_COMPRESSION_LEVEL 0.0
#define LUM_FILE_MAX_COMPRESSION_LEVEL 1.0

#endif // LUM_NATIVE_FILE_HANDLER_H