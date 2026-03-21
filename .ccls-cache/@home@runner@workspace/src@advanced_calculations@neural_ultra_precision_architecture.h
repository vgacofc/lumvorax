// NOUVEAU FICHIER : src/advanced_calculations/neural_ultra_precision_architecture.h

#ifndef NEURAL_ULTRA_PRECISION_ARCHITECTURE_H
#define NEURAL_ULTRA_PRECISION_ARCHITECTURE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declarations pour éviter inclusion circulaire
#ifndef NEURAL_COMPLEXITY_TARGET_E_DEFINED
#define NEURAL_COMPLEXITY_TARGET_E_DEFINED
typedef enum {
    NEURAL_COMPLEXITY_LOW = 0,
    NEURAL_COMPLEXITY_MEDIUM = 1,
    NEURAL_COMPLEXITY_HIGH = 2,
    NEURAL_COMPLEXITY_EXTREME = 3
} neural_complexity_target_e;
#endif

#ifndef NEURAL_ULTRA_PRECISION_CONFIG_T_DEFINED
#define NEURAL_ULTRA_PRECISION_CONFIG_T_DEFINED
typedef struct {
    neural_complexity_target_e complexity_target;
    size_t memory_capacity;
    double learning_rate;
    void* plasticity_rules; // neural_plasticity_rules_e
    bool enable_continuous_learning;
    bool enable_metaplasticity;
} neural_architecture_config_t;

typedef struct neural_ultra_precision_config_t {
    size_t precision_target_digits;      // Nombre de chiffres de précision cible
    double precision_target;             // Précision numérique cible (ex: 1e-15)
    size_t base_depth;                   // Profondeur de base du réseau
    size_t precision_layers;             // Couches dédiées à la précision
    size_t neurons_per_precision_digit;  // Neurones par chiffre de précision
    size_t input_dimensions;             // Dimensions d'entrée
    size_t output_dimensions;            // Dimensions de sortie
    double memory_scaling_factor;        // Facteur d'échelle mémoire
    double computation_scaling_factor;   // Facteur d'échelle computation
    bool enable_adaptive_precision;      // Précision adaptative
    bool enable_error_correction;        // Correction d'erreur
    uint32_t magic_number;              // Protection double-free
} neural_ultra_precision_config_t;
#endif

// Calcul architecture selon précision requise
neural_architecture_config_t* neural_calculate_ultra_precision_architecture(
    size_t input_dim, 
    size_t output_dim, 
    size_t precision_digits
);

// Calcul profondeur optimale selon complexité
size_t neural_calculate_optimal_depth(
    size_t input_dim,
    size_t output_dim,
    neural_complexity_target_e complexity
);

// Calcul largeur optimale selon dimensions
size_t neural_calculate_optimal_width(
    size_t input_dim,
    size_t output_dim,
    size_t depth
);

// Fonctions d'activation ultra-précises (sans perte numérique)
double activation_ultra_precise_tanh(double x);
double activation_ultra_precise_sigmoid(double x);  
double activation_ultra_precise_piecewise(double x);

// Constantes associées
#define NEURAL_ULTRA_PRECISION_MAGIC 0xFEEDFACE
#define MAX_PRECISION_DIGITS 50
#define DEFAULT_PRECISION_LAYERS 10
#define DEFAULT_NEURONS_PER_DIGIT 100

// Fonctions de gestion
neural_ultra_precision_config_t* neural_ultra_precision_config_create(
    size_t precision_digits, 
    size_t input_dims, 
    size_t output_dims
);

void neural_ultra_precision_config_destroy(neural_ultra_precision_config_t* config);

bool neural_ultra_precision_config_validate(const neural_ultra_precision_config_t* config);

// Validation architecture ultra-précise
bool neural_validate_ultra_precision_architecture(
    neural_architecture_config_t* config,
    size_t precision_target_digits
);

#endif // NEURAL_ULTRA_PRECISION_ARCHITECTURE_H