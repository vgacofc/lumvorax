
#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// ========== OPTIMISATIONS ENVIRONNEMENT REPLIT ==========
// Rapport 143 - Optimisations critiques adaptées conteneur Replit

// D\u00e9tection dynamique SIMD pour conteneurs Replit
#ifdef __has_include
  #if __has_include(<immintrin.h>)
    #include <immintrin.h>
    #define REPLIT_SIMD_AVAILABLE 1
  #else
    #define REPLIT_SIMD_AVAILABLE 0
  #endif
#else
  #define REPLIT_SIMD_AVAILABLE 0
#endif

// Cache line size pour optimisation conteneur (valeur s\u00e9curis\u00e9e)
#ifndef REPLIT_CACHE_LINE_SIZE
  #define REPLIT_CACHE_LINE_SIZE 64
#endif

// Limites m\u00e9moire conteneur Replit (512MB-1GB typique)
#define REPLIT_MEMORY_LIMIT_MB 768
#define REPLIT_MEMORY_WARNING_THRESHOLD (REPLIT_MEMORY_LIMIT_MB * 1024 * 1024 * 80 / 100) // 80%
#define REPLIT_MEMORY_CRITICAL_THRESHOLD (REPLIT_MEMORY_LIMIT_MB * 1024 * 1024 * 95 / 100) // 95%

// Thread pool persistent pour \u00e9viter overhead cr\u00e9ation/destruction
#define REPLIT_THREAD_POOL_SIZE 4 // 2-4 cores typique conteneur

// Cache timestamp syscalls pour r\u00e9duire overhead
#define REPLIT_TIMESTAMP_CACHE_NS 1000000 // 1ms cache

// Compression logs pour \u00e9viter saturation stockage
#define REPLIT_LOG_COMPRESSION_ENABLED 1
#define REPLIT_LOG_MAX_SIZE_MB 100

// I/O buffering optimization pour NFS storage
#define REPLIT_IO_BUFFER_SIZE (256 * 1024) // 256KB buffer

// ========== FIN OPTIMISATIONS REPLIT ==========

// FLAGS DE DÉSACTIVATION MODULES
// Désactivé par défaut - réactivation manuelle uniquement
#ifndef MODULES_QUANTIQUES_ACTIFS
#define MODULES_QUANTIQUES_ACTIFS
#endif
// #define MODULES_BLACKBOX_ACTIFS

// Constantes communes - tailles augmentées pour éviter troncations
#define MAX_STORAGE_PATH_LENGTH 4096
#define MAX_ERROR_MESSAGE_LENGTH 1024

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI
// Aucune utilisation d'emoji dans le code source ou dans les fichiers de log. 
// Toute inclusion d'emoji sera considérée comme une violation des standards de codage.

// Type unifié pour tous les résultats de stockage
typedef struct {
    bool success;
    char filename[MAX_STORAGE_PATH_LENGTH];
    size_t bytes_written;
    size_t bytes_read;
    uint32_t checksum;
    char error_message[MAX_ERROR_MESSAGE_LENGTH];
    void* transaction_ref;
    // Champs WAL extension
    uint64_t wal_sequence_assigned;
    uint64_t wal_transaction_id;
    bool wal_durability_confirmed;
    char wal_error_details[MAX_ERROR_MESSAGE_LENGTH];
    uint32_t magic_number;
} unified_storage_result_t;

// Types forensiques unifiés
typedef enum {
    FORENSIC_LEVEL_DEBUG = 0,
    FORENSIC_LEVEL_INFO = 1,
    FORENSIC_LEVEL_WARNING = 2,
    FORENSIC_LEVEL_ERROR = 3,
    FORENSIC_LEVEL_CRITICAL = 4
} unified_forensic_level_e;

// Interface forensique standardisée
void unified_forensic_log(unified_forensic_level_e level, const char* function, const char* format, ...);

// === MACROS DEBUG CONDITIONNELLES === 
// SOLUTION CRITIQUE audit forensique 2025-09-24: Éviter dégradation performance x66
// PROBLÈME: printf() debug dans lum_group_add() causait 4M appels pour 1M éléments
// SOLUTION: DEBUG_PRINTF conditionnelle - production=((void)0), debug=printf()
#ifdef DEBUG_MODE
    #define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...) ((void)0)
#endif

// === SHARED NEURAL NETWORK TYPES ===
// These types are shared across neural modules to avoid conflicts

// Activation function types (shared)
#ifndef ACTIVATION_FUNCTION_E_DEFINED
#define ACTIVATION_FUNCTION_E_DEFINED
typedef enum {
    ACTIVATION_TANH = 0,
    ACTIVATION_SIGMOID = 1,
    ACTIVATION_RELU = 2,
    ACTIVATION_GELU = 3,
    ACTIVATION_SWISH = 4,
    ACTIVATION_LEAKY_RELU = 5,
    ACTIVATION_SOFTMAX = 6,
    ACTIVATION_LINEAR = 7
} activation_function_e;
#endif

// === ADDITIONAL MODULE TYPES IDENTIFIED ===

// Audio processing types
#ifndef AUDIO_FILTER_TYPE_E_DEFINED
#define AUDIO_FILTER_TYPE_E_DEFINED
typedef enum {
    AUDIO_FILTER_LOWPASS = 0,
    AUDIO_FILTER_HIGHPASS,
    AUDIO_FILTER_BANDPASS,
    AUDIO_FILTER_NOTCH,
    AUDIO_FILTER_FFT
} audio_filter_type_e;
#endif

// Image processing types
#ifndef IMAGE_FILTER_TYPE_E_DEFINED
#define IMAGE_FILTER_TYPE_E_DEFINED
typedef enum {
    IMAGE_FILTER_BLUR = 0,
    IMAGE_FILTER_SHARPEN,
    IMAGE_FILTER_EDGE_DETECTION,
    IMAGE_FILTER_GRAYSCALE
} image_filter_type_e;
#endif

// Video codec types
#ifndef VIDEO_CODEC_TYPE_E_DEFINED
#define VIDEO_CODEC_TYPE_E_DEFINED
typedef enum {
    VIDEO_CODEC_LUM_VORAX = 0,
    VIDEO_CODEC_STANDARD
} video_codec_type_e;
#endif

// Performance classification
#ifndef PERFORMANCE_CLASS_E_DEFINED
#define PERFORMANCE_CLASS_E_DEFINED
typedef enum {
    PERFORMANCE_EXCEPTIONAL = 0,
    PERFORMANCE_SUPERIOR,
    PERFORMANCE_COMPETITIVE,
    PERFORMANCE_STANDARD
} performance_class_e;
#endif

// Optimization mechanisms
#ifndef OPACITY_MECHANISM_E_DEFINED
#define OPACITY_MECHANISM_E_DEFINED
typedef enum {
    OPACITY_COMPUTATIONAL_FOLDING = 0,
    OPACITY_SEMANTIC_SHUFFLING = 1,
    OPACITY_LOGIC_FRAGMENTATION = 2,
    OPACITY_DYNAMIC_REDIRECTION = 3,
    OPACITY_ALGORITHMIC_MORPHING = 4,
    OPACITY_CONTROL_FLOW_OBFUSCATION = 5
} opacity_mechanism_e;
#endif

// Collatz analysis types
#ifndef COLLATZ_ANALYSIS_E_DEFINED
#define COLLATZ_ANALYSIS_E_DEFINED
typedef enum {
    COLLATZ_ANALYSIS_BASIC,
    COLLATZ_ANALYSIS_STATISTICAL,
    COLLATZ_ANALYSIS_PATTERN_DETECTION,
    COLLATZ_ANALYSIS_PARALLEL_BATCH,
    COLLATZ_ANALYSIS_CONVERGENCE_STUDY,
    COLLATZ_ANALYSIS_RECORD_BREAKING
} collatz_analysis_e;
#endif

// Neural plasticity rules (shared)
#ifndef NEURAL_PLASTICITY_RULES_E_DEFINED
#define NEURAL_PLASTICITY_RULES_E_DEFINED
typedef enum {
    PLASTICITY_HEBBIAN = 0,         // Apprentissage Hebbien
    PLASTICITY_ANTI_HEBBIAN = 1,    // Anti-Hebbien
    PLASTICITY_STDP = 2,            // Spike-Timing Dependent Plasticity
    PLASTICITY_HOMEOSTATIC = 3      // Plasticité homéostatique
} neural_plasticity_rules_e;
#endif

// Neural layer structure (shared)
#ifndef NEURAL_LAYER_T_DEFINED
#define NEURAL_LAYER_T_DEFINED
typedef struct neural_layer_t {
    size_t neuron_count;        // Nombre de neurones dans cette couche
    size_t input_size;          // Nombre d'entrées par neurone
    size_t output_size;         // Nombre de sorties (= neuron_count)
    double* weights;            // Poids synaptiques [neuron_count * input_size]
    double* biases;             // Biais pour chaque neurone [neuron_count]
    double* outputs;            // Sorties calculées [neuron_count]
    double* layer_error;        // Erreurs pour backpropagation [neuron_count]
    activation_function_e activation; // Type de fonction d'activation
    uint32_t layer_id;          // Identifiant unique de couche
    uint32_t magic_number;      // Protection intégrité (0xABCDEF01)
    void* memory_address;       // Protection double-free OBLIGATOIRE
} neural_layer_t;
#endif

// === ADDITIONAL COMMON STRUCTURES ===

// Matrix types (shared across matrix_calculator and neural modules)
#ifndef MATRIX_T_DEFINED
#define MATRIX_T_DEFINED
typedef struct matrix_t {
    double* data;
    size_t rows;
    size_t cols;
    uint32_t magic_number;
    void* memory_address;
} matrix_t;
#endif

// Golden metrics structure (shared)
#ifndef GOLDEN_METRICS_T_DEFINED
#define GOLDEN_METRICS_T_DEFINED
typedef struct golden_metrics_t {
    double performance_score;
    double memory_efficiency;
    double energy_consumption;
    double scalability_factor;
    double reliability_index;
    double throughput_ratio;
    uint64_t timestamp;
    uint64_t collection_time_ns;
    void* memory_address;
    uint32_t magic_number;
} golden_metrics_t;
#endif

// Golden comparison structure (shared)
#ifndef GOLDEN_COMPARISON_T_DEFINED
#define GOLDEN_COMPARISON_T_DEFINED
typedef struct golden_comparison_t {
    golden_metrics_t* current_metrics;
    golden_metrics_t* industry_benchmark;
    double improvement_ratio;
    performance_class_e performance_class;
    char comparison_summary[256];
    double standard_ratios[5];
    performance_class_e performance_vs_standards[5];
    char detailed_analysis[5][256];
    double market_position_ratio;
    double golden_ratio_achievement;
    void* memory_address;
    uint32_t magic_number;
} golden_comparison_t;
#endif

// Computational opacity structure (shared)
#ifndef COMPUTATIONAL_OPACITY_T_DEFINED
#define COMPUTATIONAL_OPACITY_T_DEFINED
typedef struct computational_opacity_t {
    void* original_function_ptr;
    void* obfuscated_layer;
    size_t complexity_depth;
    uint64_t transformation_seed;
    bool is_active;
    void* memory_address;
    uint32_t blackbox_magic;
} computational_opacity_t;
#endif

// Blackbox configuration (shared)
#ifndef BLACKBOX_CONFIG_T_DEFINED
#define BLACKBOX_CONFIG_T_DEFINED
typedef struct blackbox_config_t {
    opacity_mechanism_e primary_mechanism;
    opacity_mechanism_e secondary_mechanism;
    double opacity_strength;
    bool enable_dynamic_morphing;
    size_t max_recursion_depth;
    uint64_t entropy_source;
    void* memory_address;
} blackbox_config_t;
#endif

#endif // COMMON_TYPES_H
