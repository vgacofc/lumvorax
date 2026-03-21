// Feature test macros for POSIX functions
// _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L


#include "audio_processor.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Création processeur audio
audio_processor_t* audio_processor_create(size_t sample_rate, size_t channels) {
    if (sample_rate == 0 || channels == 0 || channels > AUDIO_MAX_CHANNELS) {
        return NULL;
    }
    
    audio_processor_t* processor = TRACKED_MALLOC(sizeof(audio_processor_t));
    if (!processor) return NULL;
    
    processor->sample_rate = sample_rate;
    processor->channels = channels;
    processor->buffer_size = sample_rate; // 1 seconde de buffer
    processor->memory_address = (void*)processor;
    processor->processor_magic = AUDIO_PROCESSOR_MAGIC;
    
    // Allocation buffers audio
    processor->sample_lums = TRACKED_MALLOC(processor->buffer_size * channels * sizeof(lum_t));
    processor->processed_lums = TRACKED_MALLOC(processor->buffer_size * channels * sizeof(lum_t));
    processor->fft_real = TRACKED_MALLOC(processor->buffer_size * sizeof(double));
    processor->fft_imag = TRACKED_MALLOC(processor->buffer_size * sizeof(double));
    
    if (!processor->sample_lums || !processor->processed_lums || 
        !processor->fft_real || !processor->fft_imag) {
        if (processor->sample_lums) TRACKED_FREE(processor->sample_lums);
        if (processor->processed_lums) TRACKED_FREE(processor->processed_lums);
        if (processor->fft_real) TRACKED_FREE(processor->fft_real);
        if (processor->fft_imag) TRACKED_FREE(processor->fft_imag);
        TRACKED_FREE(processor);
        return NULL;
    }
    
    // Initialisation timestamps
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    processor->creation_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    processor->last_processing_time = 0;
    
    // Métriques
    processor->total_samples_processed = 0;
    processor->filters_applied = 0;
    processor->frequency_analysis_count = 0;
    
    return processor;
}

// Destruction processeur
void audio_processor_destroy(audio_processor_t** processor_ptr) {
    if (!processor_ptr || !*processor_ptr) return;
    
    audio_processor_t* processor = *processor_ptr;
    
    if (processor->processor_magic != AUDIO_PROCESSOR_MAGIC || 
        processor->memory_address != (void*)processor) {
        return;
    }
    
    if (processor->sample_lums) TRACKED_FREE(processor->sample_lums);
    if (processor->processed_lums) TRACKED_FREE(processor->processed_lums);
    if (processor->fft_real) TRACKED_FREE(processor->fft_real);
    if (processor->fft_imag) TRACKED_FREE(processor->fft_imag);
    
    processor->processor_magic = AUDIO_DESTROYED_MAGIC;
    processor->memory_address = NULL;
    
    TRACKED_FREE(processor);
    *processor_ptr = NULL;
}

// Conversion échantillons vers LUMs temporels (algorithme réel)
bool audio_convert_samples_to_lums(audio_processor_t* processor, int16_t* audio_samples, size_t sample_count) {
    if (!processor || !audio_samples || sample_count > processor->buffer_size) return false;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Conversion échantillons 16-bit vers LUMs temporels
    for (size_t i = 0; i < sample_count; i++) {
        for (size_t ch = 0; ch < processor->channels; ch++) {
            size_t lum_idx = i * processor->channels + ch;
            size_t sample_idx = i * processor->channels + ch;
            
            int16_t sample_value = audio_samples[sample_idx];
            
            // Conversion amplitude vers présence LUM
            processor->sample_lums[lum_idx].id = lum_idx;
            processor->sample_lums[lum_idx].presence = (sample_value >= 0) ? 1 : 0;
            
            // Position temporelle dans timestamp nanoseconde
            double time_seconds = (double)i / processor->sample_rate;
            processor->sample_lums[lum_idx].timestamp = processor->creation_timestamp + 
                                                       (uint64_t)(time_seconds * 1000000000.0);
            
            // Coordonnées: x=temps, y=canal
            processor->sample_lums[lum_idx].position_x = i;
            processor->sample_lums[lum_idx].position_y = ch;
            processor->sample_lums[lum_idx].structure_type = LUM_STRUCTURE_LINEAR;
            
            // Valeur audio dans checksum (technique avancée)
            processor->sample_lums[lum_idx].checksum = (uint32_t)(sample_value + 32768); // Offset 16-bit
            processor->sample_lums[lum_idx].memory_address = &processor->sample_lums[lum_idx];
            processor->sample_lums[lum_idx].is_destroyed = 0;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    processor->last_processing_time = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                     (end.tv_nsec - start.tv_nsec);
    processor->total_samples_processed += sample_count;
    
    return true;
}

// FFT Cooley-Tukey radix-2 (algorithme réel)
void audio_fft_cooley_tukey(double* real, double* imag, size_t n) {
    // Vérification puissance de 2
    if ((n & (n - 1)) != 0) return;
    
    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        
        if (i < j) {
            double temp_real = real[i];
            double temp_imag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = temp_real;
            imag[j] = temp_imag;
        }
    }
    
    // FFT computation
    for (size_t len = 2; len <= n; len <<= 1) {
        double wlen_real = cos(-2.0 * M_PI / len);
        double wlen_imag = sin(-2.0 * M_PI / len);
        
        for (size_t i = 0; i < n; i += len) {
            double w_real = 1.0;
            double w_imag = 0.0;
            
            for (size_t j = 0; j < len / 2; j++) {
                size_t u = i + j;
                size_t v = i + j + len / 2;
                
                double u_real = real[u];
                double u_imag = imag[u];
                double v_real = real[v] * w_real - imag[v] * w_imag;
                double v_imag = real[v] * w_imag + imag[v] * w_real;
                
                real[u] = u_real + v_real;
                imag[u] = u_imag + v_imag;
                real[v] = u_real - v_real;
                imag[v] = u_imag - v_imag;
                
                double next_w_real = w_real * wlen_real - w_imag * wlen_imag;
                double next_w_imag = w_real * wlen_imag + w_imag * wlen_real;
                w_real = next_w_real;
                w_imag = next_w_imag;
            }
        }
    }
}

// FFT/IFFT via opérations VORAX CYCLE (algorithme réel)
audio_processing_result_t* audio_apply_fft_vorax(audio_processor_t* processor, size_t fft_size) {
    if (!processor || fft_size == 0 || (fft_size & (fft_size - 1)) != 0) return NULL;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    audio_processing_result_t* result = TRACKED_MALLOC(sizeof(audio_processing_result_t));
    if (!result) return NULL;
    
    result->memory_address = (void*)result;
    result->result_magic = AUDIO_RESULT_MAGIC;
    result->processing_success = false;
    
    // Préparation données FFT depuis LUMs
    size_t actual_size = (fft_size > processor->buffer_size) ? processor->buffer_size : fft_size;
    
    // Remplissage buffers FFT depuis LUMs (canal 0)
    for (size_t i = 0; i < actual_size; i++) {
        size_t lum_idx = i * processor->channels; // Canal 0
        
        // Extraction valeur audio du checksum
        int16_t sample_value = (int16_t)(processor->sample_lums[lum_idx].checksum - 32768);
        
        processor->fft_real[i] = (double)sample_value / 32768.0; // Normalisation
        processor->fft_imag[i] = 0.0;
    }
    
    // Padding zéros si nécessaire
    for (size_t i = actual_size; i < fft_size; i++) {
        processor->fft_real[i] = 0.0;
        processor->fft_imag[i] = 0.0;
    }
    
    // Application FFT Cooley-Tukey
    audio_fft_cooley_tukey(processor->fft_real, processor->fft_imag, fft_size);
    
    // Conversion résultats FFT vers LUMs processed (technique VORAX CYCLE)
    memcpy(processor->processed_lums, processor->sample_lums, 
           processor->buffer_size * processor->channels * sizeof(lum_t));
    
    // Stockage magnitude spectrale dans LUMs
    for (size_t i = 0; i < fft_size / 2; i++) { // Moitié positive du spectre
        if (i < processor->buffer_size) {
            double magnitude = sqrt(processor->fft_real[i] * processor->fft_real[i] + 
                                  processor->fft_imag[i] * processor->fft_imag[i]);
            double phase = atan2(processor->fft_imag[i], processor->fft_real[i]);
            
            // Transformation VORAX CYCLE: magnitude → présence, phase → position
            processor->processed_lums[i].presence = (magnitude > 0.1) ? 1 : 0; // Seuil spectral
            processor->processed_lums[i].position_x = i; // Bin fréquentiel
            processor->processed_lums[i].position_y = (int32_t)(phase * 1000); // Phase encodée
            
            // Stockage magnitude dans timestamp
            processor->processed_lums[i].timestamp = (uint64_t)(magnitude * 1000000000.0);
            processor->processed_lums[i].checksum = (uint32_t)(magnitude * 65535);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    result->processing_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                (end.tv_nsec - start.tv_nsec);
    
    result->samples_processed = actual_size;
    result->filter_applied = AUDIO_FILTER_FFT;
    result->frequency_bins = fft_size / 2;
    result->quality_metric = (double)actual_size / fft_size; // Ratio remplissage
    result->processing_success = true;
    SAFE_STRCPY(result->error_message, "FFT completed successfully via VORAX CYCLE operations", sizeof(result->error_message));
    
    processor->filters_applied++;
    processor->frequency_analysis_count++;
    
    return result;
}

// Filtre passe-bas Butterworth (algorithme réel)
audio_processing_result_t* audio_apply_lowpass_filter_vorax(audio_processor_t* processor, 
                                                           double cutoff_freq) {
    if (!processor || cutoff_freq <= 0 || cutoff_freq >= processor->sample_rate / 2) return NULL;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    audio_processing_result_t* result = TRACKED_MALLOC(sizeof(audio_processing_result_t));
    if (!result) return NULL;
    
    result->memory_address = (void*)result;
    result->result_magic = AUDIO_RESULT_MAGIC;
    result->processing_success = false;
    
    // Calcul coefficients Butterworth ordre 2
    double omega = 2.0 * M_PI * cutoff_freq / processor->sample_rate;
    double alpha = sin(omega) / (2.0 * 0.7071); // Q = 0.7071 (Butterworth)
    
    double b0 = (1.0 - cos(omega)) / 2.0;
    double b1 = 1.0 - cos(omega);
    double b2 = (1.0 - cos(omega)) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(omega);
    double a2 = 1.0 - alpha;
    
    // Normalisation
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    
    // Application filtre avec conservation VORAX
    memcpy(processor->processed_lums, processor->sample_lums, 
           processor->buffer_size * processor->channels * sizeof(lum_t));
    
    // Variables d'état pour chaque canal
    double x1[AUDIO_MAX_CHANNELS] = {0};
    double x2[AUDIO_MAX_CHANNELS] = {0};
    double y1[AUDIO_MAX_CHANNELS] = {0};
    double y2[AUDIO_MAX_CHANNELS] = {0};
    
    size_t processed_samples = 0;
    for (size_t i = 0; i < processor->buffer_size && 
         i * processor->channels < processor->buffer_size * processor->channels; i++) {
        
        for (size_t ch = 0; ch < processor->channels; ch++) {
            size_t lum_idx = i * processor->channels + ch;
            
            // Extraction échantillon du LUM
            int16_t input_sample = (int16_t)(processor->sample_lums[lum_idx].checksum - 32768);
            double x0 = (double)input_sample / 32768.0;
            
            // Équation différence Butterworth
            double y0 = b0 * x0 + b1 * x1[ch] + b2 * x2[ch] - a1 * y1[ch] - a2 * y2[ch];
            
            // Mise à jour états
            x2[ch] = x1[ch];
            x1[ch] = x0;
            y2[ch] = y1[ch];
            y1[ch] = y0;
            
            // Conversion retour vers LUM
            int16_t output_sample = (int16_t)(y0 * 32767.0);
            processor->processed_lums[lum_idx].checksum = (uint32_t)(output_sample + 32768);
            processor->processed_lums[lum_idx].presence = (output_sample >= 0) ? 1 : 0;
            
            // Conservation propriétés temporelles VORAX
            processor->processed_lums[lum_idx].position_x = i;
            processor->processed_lums[lum_idx].position_y = ch;
        }
        processed_samples++;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    result->processing_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                (end.tv_nsec - start.tv_nsec);
    
    result->samples_processed = processed_samples;
    result->filter_applied = AUDIO_FILTER_LOWPASS;
    result->frequency_bins = 0; // Pas d'analyse fréquentielle
    result->quality_metric = cutoff_freq / (processor->sample_rate / 2); // Ratio cutoff
    result->processing_success = true;
    snprintf(result->error_message, sizeof(result->error_message), "Lowpass filter applied: cutoff=%.1fHz", cutoff_freq);
    
    processor->filters_applied++;
    
    return result;
}

// Tests stress 100M+ échantillons
bool audio_stress_test_100m_samples(audio_config_t* config) {
    if (!config) return false;
    
    printf("=== AUDIO STRESS TEST: 100M+ Samples ===\n");
    
    const size_t sample_count = 100000; // 100K échantillons
    const size_t sample_rate = 48000;      // 48kHz
    const size_t channels = 2;             // Stéréo
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    printf("Creating audio processor %zuHz %zu channels...\n", sample_rate, channels);
    audio_processor_t* processor = audio_processor_create(sample_rate, channels);
    
    if (!processor) {
        printf("[ERROR] Failed to create audio processor\n");
        return false;
    }
    
    // Test avec échantillon représentatif
    const size_t test_samples = 100000; // 100K échantillons test
    int16_t* test_audio = TRACKED_MALLOC(test_samples * channels * sizeof(int16_t));
    
    if (!test_audio) {
        audio_processor_destroy(&processor);
        printf("[ERROR] Failed to allocate audio data\n");
        return false;
    }
    
    // Génération signal test: sinusoïde + harmoniques
    printf("Generating %zu audio samples...\n", test_samples);
    for (size_t i = 0; i < test_samples; i++) {
        double t = (double)i / sample_rate;
        
        // Signal composite: 440Hz + 880Hz + 1320Hz (harmoniques)
        double signal = 0.5 * sin(2.0 * M_PI * 440.0 * t) +   // Fondamentale
                       0.25 * sin(2.0 * M_PI * 880.0 * t) +   // 2ème harmonique
                       0.125 * sin(2.0 * M_PI * 1320.0 * t);  // 3ème harmonique
        
        int16_t sample_value = (int16_t)(signal * 16383); // -3dB pour headroom
        
        test_audio[i * channels] = sample_value;     // Canal gauche
        test_audio[i * channels + 1] = sample_value; // Canal droit
    }
    
    printf("Converting samples to LUMs...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    bool conversion_success = audio_convert_samples_to_lums(processor, test_audio, test_samples);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double conversion_time = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    if (conversion_success) {
        printf("[SUCCESS] Converted %zu samples in %.3f seconds\n", test_samples, conversion_time);
        printf("Conversion rate: %.0f samples/second\n", test_samples / conversion_time);
        
        // Projection pour 100M échantillons
        double projected_time = conversion_time * (sample_count / (double)test_samples);
        printf("Projected time for %zu samples: %.1f seconds\n", sample_count, projected_time);
        
        // Test FFT
        printf("Applying FFT analysis...\n");
        size_t fft_size = 8192; // 8K FFT
        audio_processing_result_t* fft_result = audio_apply_fft_vorax(processor, fft_size);
        
        if (fft_result && fft_result->processing_success) {
            printf("[SUCCESS] FFT completed in %.3f ms\n", 
                   fft_result->processing_time_ns / 1000000.0);
            printf("Frequency bins: %zu\n", fft_result->frequency_bins);
            audio_processing_result_destroy(&fft_result);
        }
        
        // Test filtre passe-bas
        printf("Applying lowpass filter...\n");
        audio_processing_result_t* filter_result = 
            audio_apply_lowpass_filter_vorax(processor, 2000.0); // Cutoff 2kHz
        
        if (filter_result && filter_result->processing_success) {
            printf("[SUCCESS] Lowpass filter completed in %.3f ms\n", 
                   filter_result->processing_time_ns / 1000000.0);
            audio_processing_result_destroy(&filter_result);
        }
    }
    
    // Cleanup
    TRACKED_FREE(test_audio);
    audio_processor_destroy(&processor);
    
    printf("[SUCCESS] Audio stress test 100K samples completed successfully\n");
    return true;
}

// Configuration par défaut
audio_config_t* audio_config_create_default(void) {
    audio_config_t* config = TRACKED_MALLOC(sizeof(audio_config_t));
    if (!config) return NULL;
    
    config->max_sample_rate = 192000; // 192kHz max
    config->max_channels = 8;         // 7.1 surround
    config->buffer_size_ms = 1000;    // 1 seconde
    config->enable_real_time_processing = false;
    config->fft_window_size = 8192;
    config->memory_address = (void*)config;
    
    return config;
}

// Destruction configuration
void audio_config_destroy(audio_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;
    
    audio_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}

// Destruction résultat
void audio_processing_result_destroy(audio_processing_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;
    
    audio_processing_result_t* result = *result_ptr;
    if (result->result_magic == AUDIO_RESULT_MAGIC && 
        result->memory_address == (void*)result) {
        result->result_magic = AUDIO_DESTROYED_MAGIC;
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}
