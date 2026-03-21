
#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../lum/lum_core.h"
#include "../common/common_types.h" // Pour audio_filter_type_e

// Constantes
#define AUDIO_MAX_CHANNELS 8
#define AUDIO_MAX_SAMPLE_RATE 192000
#define AUDIO_PROCESSOR_MAGIC 0x41554450 // "AUDP"
#define AUDIO_DESTROYED_MAGIC 0xDEADBEEF
#define AUDIO_RESULT_MAGIC 0x41554452    // "AUDR"

// audio_filter_type_e défini dans common_types.h

// Structure processeur audio
typedef struct {
    size_t sample_rate;
    size_t channels;
    size_t buffer_size;
    
    lum_t* sample_lums;     // LUMs pour échantillons
    lum_t* processed_lums;  // LUMs après traitement
    
    double* fft_real;       // Buffer FFT partie réelle
    double* fft_imag;       // Buffer FFT partie imaginaire
    
    uint64_t creation_timestamp;
    uint64_t last_processing_time;
    
    size_t total_samples_processed;
    size_t filters_applied;
    size_t frequency_analysis_count;
    
    void* memory_address;
    uint32_t processor_magic;
} audio_processor_t;

// Configuration
typedef struct {
    size_t max_sample_rate;
    size_t max_channels;
    size_t buffer_size_ms;
    bool enable_real_time_processing;
    size_t fft_window_size;
    void* memory_address;
} audio_config_t;

// Résultat traitement
typedef struct {
    bool processing_success;
    uint64_t processing_time_ns;
    size_t samples_processed;
    audio_filter_type_e filter_applied;
    size_t frequency_bins;
    double quality_metric;
    char error_message[256];
    void* memory_address;
    uint32_t result_magic;
} audio_processing_result_t;

// Fonctions publiques
audio_processor_t* audio_processor_create(size_t sample_rate, size_t channels);
void audio_processor_destroy(audio_processor_t** processor_ptr);

bool audio_convert_samples_to_lums(audio_processor_t* processor, int16_t* audio_samples, size_t sample_count);
audio_processing_result_t* audio_apply_fft_vorax(audio_processor_t* processor, size_t fft_size);
audio_processing_result_t* audio_apply_lowpass_filter_vorax(audio_processor_t* processor, double cutoff_freq);

bool audio_stress_test_100m_samples(audio_config_t* config);

audio_config_t* audio_config_create_default(void);
void audio_config_destroy(audio_config_t** config_ptr);
void audio_processing_result_destroy(audio_processing_result_t** result_ptr);

#endif // AUDIO_PROCESSOR_H
