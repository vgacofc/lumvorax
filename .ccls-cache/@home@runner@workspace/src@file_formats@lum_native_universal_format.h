#ifndef LUM_NATIVE_UNIVERSAL_FORMAT_H
#define LUM_NATIVE_UNIVERSAL_FORMAT_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Magic numbers pour protection et validation format
#define LUM_UNIVERSAL_MAGIC 0x4C554D55  // "LUMU" 
#define LUM_HEADER_MAGIC 0x4C484452     // "LHDR"
#define LUM_DATA_MAGIC 0x4C444154       // "LDAT"
// LUM_DESTROYED_MAGIC déjà défini dans lum_core.h

// CORRECTION RAPPORT 117: Version format configurable
typedef struct {
    uint16_t major;
    uint16_t minor;
    const char* build_info;
} lum_format_version_t;

lum_format_version_t lum_get_format_version(void);
bool lum_set_format_version(uint16_t major, uint16_t minor, const char* build_info);

// Types de contenus supportés dans fichier .lum
typedef enum {
    LUM_CONTENT_TEXT,           // Données texte UTF-8
    LUM_CONTENT_JSON,           // Données JSON structurées
    LUM_CONTENT_CSV,            // Données tabulaires CSV
    LUM_CONTENT_IMAGE_RGB24,    // Images RGB 24-bit
    LUM_CONTENT_IMAGE_RGBA32,   // Images RGBA 32-bit
    LUM_CONTENT_IMAGE_GRAY8,    // Images grayscale 8-bit
    LUM_CONTENT_AUDIO_PCM16,    // Audio PCM 16-bit
    LUM_CONTENT_AUDIO_PCM32,    // Audio PCM 32-bit
    LUM_CONTENT_VIDEO_H264,     // Vidéo H.264 encoded
    LUM_CONTENT_VIDEO_RAW,      // Vidéo raw frames
    LUM_CONTENT_SOM_KOHONEN,    // Self-Organizing Map data
    LUM_CONTENT_NEURAL_WEIGHTS, // Poids réseaux neuronaux
    LUM_CONTENT_QUANTUM_STATES, // États quantiques
    LUM_CONTENT_BINARY_BLOB,    // Données binaires génériques
    LUM_CONTENT_LUM_NATIVE,     // Structures LUM natives
    LUM_CONTENT_COUNT
} lum_content_type_e;

// CORRECTION RAPPORT 117: Types de contenus configurables
const char* lum_get_content_type_name(lum_content_type_e type);
bool lum_set_custom_content_type_names(const char* names[], size_t count);

// Métadonnées fichier .lum
typedef struct {
    uint32_t magic_header;              // LUM_HEADER_MAGIC
    uint16_t format_version_major;      // Version majeure format
    uint16_t format_version_minor;      // Version mineure format
    uint64_t file_creation_timestamp;   // Timestamp création UTC nanoseconds
    uint64_t total_file_size_bytes;     // Taille totale fichier
    uint32_t content_sections_count;    // Nombre sections contenus
    uint32_t lum_structures_count;      // Nombre structures LUM encodées
    char creator_signature[32];         // Signature créateur
    uint32_t checksum_crc32;           // Checksum CRC32 validation
    uint8_t compression_type;          // Type compression (0=none, 1=zlib, 2=lz4)
    uint8_t encryption_type;           // Type chiffrement (0=none, 1=AES256)
    uint8_t reserved[6];              // Réservé extensions futures
    void* memory_address;             // Protection double-free
} lum_file_header_t;

// Section de contenu dans fichier .lum
typedef struct {
    uint32_t magic_data;              // LUM_DATA_MAGIC
    lum_content_type_e content_type;  // Type contenu section
    uint64_t section_offset_bytes;    // Offset section dans fichier
    uint64_t section_size_bytes;      // Taille section (non compressée)
    uint64_t compressed_size_bytes;   // Taille compressée (si applicable)
    uint32_t lum_mapping_count;       // Nombre LUMs associés à cette section
    char section_name[64];            // Nom descriptif section
    char metadata_json[256];          // Métadonnées JSON additionnelles
    uint32_t section_checksum;        // Checksum section
    void* memory_address;             // Protection double-free
} lum_content_section_t;

// Mappage LUM vers données dans section
typedef struct {
    uint32_t lum_id;                  // ID LUM associé
    int32_t lum_position_x;           // Position spatiale X
    int32_t lum_position_y;           // Position spatiale Y
    uint64_t data_offset_in_section;  // Offset données dans section
    uint64_t data_size_bytes;         // Taille données pour ce LUM
    uint8_t lum_presence;             // État présence LUM (0/1)
    uint8_t structure_type;           // Type structure LUM
    uint16_t reserved;                // Réservé alignement
} lum_data_mapping_t;

// Gestionnaire fichier .lum universel
typedef struct {
    lum_file_header_t* header;                    // Header fichier
    lum_content_section_t** sections;             // Array sections contenus
    lum_data_mapping_t** lum_mappings;            // Mappings LUM par section
    FILE* file_handle;                           // Handle fichier ouvert
    char filepath[512];                          // Chemin fichier complet
    bool is_read_only;                           // Mode lecture seule
    bool is_dirty;                              // Modifications non sauvées
    lum_group_t* cached_lums;                    // Cache LUMs chargés
    void* memory_address;                        // Protection double-free
    uint32_t manager_magic;                      // Protection corruption
} lum_universal_file_manager_t;

// Résultat opération fichier
typedef struct {
    bool operation_success;                      // Succès opération
    uint64_t bytes_processed;                    // Bytes traités
    uint32_t lums_affected;                      // LUMs affectés
    double processing_time_ms;                   // Temps traitement
    char error_message[256];                     // Message erreur si échec
    lum_content_type_e detected_content_type;    // Type contenu détecté
    void* memory_address;                        // Protection double-free
} lum_file_operation_result_t;

// === FONCTIONS CRÉATION ET GESTION FICHIERS ===

// Création manager fichier .lum universel
lum_universal_file_manager_t* lum_universal_file_create(const char* filepath);

// Ouverture fichier .lum existant
lum_universal_file_manager_t* lum_universal_file_open(const char* filepath, bool read_only);

// Sauvegarde et fermeture fichier
bool lum_universal_file_save_and_close(lum_universal_file_manager_t** manager_ptr);

// Destruction manager (sans sauvegarder)
void lum_universal_file_destroy(lum_universal_file_manager_t** manager_ptr);

// === FONCTIONS AJOUT CONTENUS MULTI-FORMATS ===

// Ajout contenu texte UTF-8
lum_file_operation_result_t* lum_add_text_content(lum_universal_file_manager_t* manager,
                                                  const char* text_data,
                                                  size_t text_length,
                                                  const char* section_name,
                                                  lum_group_t* associated_lums);

// Ajout contenu JSON structuré
lum_file_operation_result_t* lum_add_json_content(lum_universal_file_manager_t* manager,
                                                  const char* json_string,
                                                  const char* section_name,
                                                  lum_group_t* associated_lums);

// Ajout contenu CSV tabulaire
lum_file_operation_result_t* lum_add_csv_content(lum_universal_file_manager_t* manager,
                                                 const char* csv_data,
                                                 size_t csv_length,
                                                 const char* section_name,
                                                 lum_group_t* associated_lums);

// Ajout image RGB24
lum_file_operation_result_t* lum_add_image_rgb24(lum_universal_file_manager_t* manager,
                                                 const uint8_t* image_data,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 const char* section_name,
                                                 lum_group_t* associated_lums);

// Ajout image RGBA32
lum_file_operation_result_t* lum_add_image_rgba32(lum_universal_file_manager_t* manager,
                                                  const uint8_t* image_data,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  const char* section_name,
                                                  lum_group_t* associated_lums);

// Ajout audio PCM
lum_file_operation_result_t* lum_add_audio_pcm(lum_universal_file_manager_t* manager,
                                               const void* audio_data,
                                               size_t audio_size,
                                               uint32_t sample_rate,
                                               uint16_t channels,
                                               uint16_t bits_per_sample,
                                               const char* section_name,
                                               lum_group_t* associated_lums);

// Ajout vidéo (frames ou encoded)
lum_file_operation_result_t* lum_add_video_content(lum_universal_file_manager_t* manager,
                                                   const uint8_t* video_data,
                                                   size_t video_size,
                                                   uint32_t width,
                                                   uint32_t height,
                                                   uint32_t fps,
                                                   lum_content_type_e video_format,
                                                   const char* section_name,
                                                   lum_group_t* associated_lums);

// Ajout données SOM (Self-Organizing Map)
lum_file_operation_result_t* lum_add_som_data(lum_universal_file_manager_t* manager,
                                              const double* som_weights,
                                              uint32_t som_width,
                                              uint32_t som_height,
                                              uint32_t input_dimension,
                                              const char* section_name,
                                              lum_group_t* associated_lums);

// Ajout poids réseau neuronal
lum_file_operation_result_t* lum_add_neural_weights(lum_universal_file_manager_t* manager,
                                                    const double* weights,
                                                    size_t weights_count,
                                                    const double* biases,
                                                    size_t biases_count,
                                                    const char* layer_name,
                                                    lum_group_t* associated_lums);

// Ajout données binaires génériques
lum_file_operation_result_t* lum_add_binary_blob(lum_universal_file_manager_t* manager,
                                                 const void* binary_data,
                                                 size_t data_size,
                                                 const char* section_name,
                                                 lum_group_t* associated_lums);

// === FONCTIONS EXTRACTION CONTENUS ===

// Extraction contenu par nom section
lum_file_operation_result_t* lum_extract_content_by_name(lum_universal_file_manager_t* manager,
                                                         const char* section_name,
                                                         void** extracted_data,
                                                         size_t* data_size);

// Extraction contenu par type
lum_file_operation_result_t* lum_extract_content_by_type(lum_universal_file_manager_t* manager,
                                                         lum_content_type_e content_type,
                                                         void** extracted_data,
                                                         size_t* data_size);

// Extraction LUMs associés à section
lum_group_t* lum_extract_associated_lums(lum_universal_file_manager_t* manager,
                                         const char* section_name);

// === FONCTIONS CONVERSION ET INTEROPÉRABILITÉ ===

// Conversion fichier texte vers .lum
bool lum_convert_text_file_to_lum(const char* input_text_file,
                                  const char* output_lum_file);

// Conversion fichier JSON vers .lum
bool lum_convert_json_file_to_lum(const char* input_json_file,
                                  const char* output_lum_file);

// Conversion fichier CSV vers .lum
bool lum_convert_csv_file_to_lum(const char* input_csv_file,
                                 const char* output_lum_file);

// Conversion image vers .lum
bool lum_convert_image_file_to_lum(const char* input_image_file,
                                   const char* output_lum_file);

// Export .lum vers formats standards
bool lum_export_to_json(const char* input_lum_file, const char* output_json_file);
bool lum_export_to_csv(const char* input_lum_file, const char* output_csv_file);

// === FONCTIONS VALIDATION ET INTÉGRITÉ ===

// Validation intégrité fichier .lum
bool lum_validate_file_integrity(const char* lum_file_path,
                                char* validation_report,
                                size_t report_size);

// Calcul checksum fichier
uint32_t lum_calculate_file_checksum(const char* lum_file_path);

// Réparation fichier .lum corrompu
bool lum_repair_corrupted_file(const char* lum_file_path,
                              const char* repaired_file_path);

// === FONCTIONS COMPRESSION ET CHIFFREMENT ===

// Activation compression sur fichier
bool lum_enable_compression(lum_universal_file_manager_t* manager,
                           uint8_t compression_type);

// Activation chiffrement AES256
bool lum_enable_encryption(lum_universal_file_manager_t* manager,
                          const char* encryption_key,
                          size_t key_length);

// === FONCTIONS STRESS TEST ET PERFORMANCE ===

// Test stress fichier .lum avec 100M+ éléments
bool lum_stress_test_100m_elements(const char* test_file_path);

// Benchmark performance I/O fichier
bool lum_benchmark_file_performance(const char* test_file_path,
                                   size_t test_data_size,
                                   double* read_speed_mbps,
                                   double* write_speed_mbps);

// === FONCTIONS MÉTADONNÉES ET INFORMATIONS ===

// Information détaillée fichier .lum
typedef struct {
    char filepath[512];
    uint64_t file_size_bytes;
    uint32_t sections_count;
    uint32_t total_lums_count;
    char format_version[16];
    char creation_timestamp[32];
    lum_content_type_e* content_types;
    char** section_names;
    uint64_t* section_sizes;
} lum_file_info_t;

// Obtenir informations complètes fichier
lum_file_info_t* lum_get_file_info(const char* lum_file_path);

// Libération informations fichier
void lum_file_info_destroy(lum_file_info_t** info_ptr);

#endif // LUM_NATIVE_UNIVERSAL_FORMAT_H