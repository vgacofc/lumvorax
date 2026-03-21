
#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

// ORDRE STRICT D'INCLUSION SELON PROMPT.TXT
#include "../common/common_types.h"
#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Constantes
#define IMAGE_MAX_DIMENSION 65536
#define IMAGE_MAX_CHANNELS 4
#define IMAGE_PROCESSOR_MAGIC 0x494D4750 // "IMGP"
#define IMAGE_DESTROYED_MAGIC 0xDEADBEEF
#define IMAGE_RESULT_MAGIC 0x494D4752    // "IMGR"

// Types de filtres DÉJÀ définis dans common_types.h

// Structure processeur d'images
typedef struct {
    size_t width;
    size_t height;
    size_t pixel_count;
    
    lum_t* pixel_lums;      // LUMs pour pixels
    lum_t* processed_lums;  // LUMs après traitement
    
    uint64_t creation_timestamp;
    uint64_t last_processing_time;
    
    size_t total_pixels_processed;
    size_t filters_applied;
    double compression_ratio;
    
    void* memory_address;
    uint32_t processor_magic;
} image_processor_t;

// Configuration
typedef struct {
    size_t max_image_size_mb;
    bool enable_gpu_acceleration;
    double default_filter_quality;
    int compression_level;
    bool enable_parallel_processing;
    void* memory_address;
} image_config_t;

// Résultat traitement
typedef struct {
    bool processing_success;
    uint64_t processing_time_ns;
    size_t pixels_processed;
    image_filter_type_e filter_applied;
    double quality_metric;
    char error_message[256];
    void* memory_address;
    uint32_t result_magic;
} image_processing_result_t;

// Fonctions publiques
image_processor_t* image_processor_create(size_t width, size_t height);
void image_processor_destroy(image_processor_t** processor_ptr);

bool image_convert_pixels_to_lums(image_processor_t* processor, uint8_t* rgb_data);
image_processing_result_t* image_apply_gaussian_blur_vorax(image_processor_t* processor, double sigma);
image_processing_result_t* image_apply_edge_detection_vorax(image_processor_t* processor);

bool image_stress_test_100m_pixels(image_config_t* config);

image_config_t* image_config_create_default(void);
void image_config_destroy(image_config_t** config_ptr);
void image_processing_result_destroy(image_processing_result_t** result_ptr);

#endif // IMAGE_PROCESSOR_H
