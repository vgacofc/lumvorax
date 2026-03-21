#ifndef NEURAL_NETWORK_PROCESSOR_H
#define NEURAL_NETWORK_PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../lum/lum_core.h"
#include "../common/common_types.h"

// Neural plasticity rules defined in common_types.h

// Structure traçage activations neuronales
typedef struct {
    size_t layer_id;
    size_t neuron_count;
    double* hidden_activations;
    double* gradients_trace;
    uint64_t forward_pass_timestamp;
    uint64_t backward_pass_timestamp;
    char activation_function_name[64];
    double layer_loss;
    void* memory_address;
    uint32_t trace_magic;
} neural_activation_trace_t;

// Module Processeur Réseau de Neurones pour LUM/VORAX
// Conforme prompt.txt - nouveau module calculs avancés

// Neurone LUM avec apprentissage spatial
typedef struct {
    lum_t base_lum;               // LUM de base avec position spatiale
    double* weights;              // Poids synaptiques
    size_t input_count;           // Nombre d'entrées
    double bias;                  // Biais du neurone
    double activation_threshold;  // Seuil d'activation
    double learning_rate;         // Taux d'apprentissage
    uint64_t fire_count;          // Nombre d'activations
    double* gradient;             // Gradient pour backpropagation
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint32_t neuron_magic;        // Validation intégrité
    bool is_active;               // État d'activation actuel
} neural_lum_t;

// Activation function types defined in common_types.h

// neural_layer_t structure is defined in common_types.h

// Réseau de neurones complet
typedef struct {
    neural_layer_t** layers;      // Array de couches
    size_t layer_count;           // Nombre de couches
    size_t num_layers;            // Alias pour layer_count (compatibilité tests)
    double* input_data;           // Données d'entrée
    double* output_data;          // Données de sortie
    double* target_data;          // Données cibles (entraînement)
    size_t input_size;            // Taille entrée réseau
    size_t output_size;           // Taille sortie réseau
    double global_learning_rate;  // Taux d'apprentissage global
    uint64_t epoch_count;         // Nombre d'époques d'entraînement
    double total_loss;            // Perte totale actuelle
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint64_t timestamp_created;   // Horodatage création
} neural_network_t;

// Résultat traitement réseau
typedef struct {
    bool success;                 // Indicateur de succès (compatibilité tests)
    double* predictions;          // Prédictions du réseau
    double* output_data;          // Alias pour predictions (compatibilité tests)
    size_t prediction_count;      // Nombre de prédictions
    double accuracy;              // Précision sur jeu de test
    double loss;                  // Fonction de perte
    uint64_t training_time_ns;    // Temps d'entraînement (ns)
    uint64_t inference_time_ns;   // Temps d'inférence (ns)
    size_t neurons_fired;         // Nombre de neurones activés
    bool convergence_reached;     // Convergence atteinte
    char error_message[256];      // Message d'erreur
    void* memory_address;         // Protection double-free OBLIGATOIRE
} neural_result_t;

// Configuration réseau de neurones
typedef struct {
    size_t max_epochs;            // Époques maximales
    double convergence_threshold; // Seuil de convergence
    bool use_momentum;            // Utilisation momentum
    double momentum_coefficient;  // Coefficient momentum
    bool use_dropout;             // Utilisation dropout
    double dropout_rate;          // Taux de dropout [0,1]
    bool use_batch_normalization; // Normalisation par batch
    size_t batch_size;            // Taille des batchs
    bool enable_gpu_acceleration; // Accélération GPU
    void* memory_address;         // Protection double-free OBLIGATOIRE
} neural_config_t;

// Fonctions principales
neural_lum_t* neural_lum_create(int32_t x, int32_t y, size_t input_count, activation_function_e activation);
void neural_lum_destroy(neural_lum_t** neuron_ptr);
neural_layer_t* neural_layer_create(size_t neuron_count, size_t input_size, activation_function_e activation);
void neural_layer_destroy(neural_layer_t** layer_ptr);
neural_network_t* neural_network_create(size_t* layer_sizes, size_t layer_count);
void neural_network_destroy(neural_network_t** network_ptr);

// Opérations réseau
double neural_lum_activate(neural_lum_t* neuron, double* inputs, activation_function_e function);
bool neural_layer_forward_pass(neural_layer_t* layer, double* inputs);
bool neural_layer_backward_pass(neural_layer_t* layer, double* errors, neural_config_t* config);
neural_result_t* neural_network_train(neural_network_t* network, double** training_data, double** target_data, size_t sample_count, neural_config_t* config);
neural_result_t* neural_network_predict(neural_network_t* network, double* input_data, neural_config_t* config);

// Fonctions utilisées dans les tests
double* neural_network_forward(neural_network_t* network, double* input_data);
double neural_network_backward(neural_network_t* network, double* output, double* target);

// Algorithmes d'apprentissage avancés
bool neural_apply_backpropagation(neural_network_t* network, double* target_output, neural_config_t* config);
bool neural_apply_momentum_update(neural_network_t* network, neural_config_t* config);
bool neural_apply_dropout(neural_layer_t* layer, double dropout_rate);
bool neural_apply_batch_normalization(neural_layer_t* layer);

// Fonctions d'activation
double activation_sigmoid(double x);
double activation_tanh(double x);
double activation_relu(double x);
double activation_leaky_relu(double x, double alpha);
void activation_softmax(double* inputs, double* outputs, size_t size);
double activation_swish(double x);
double activation_gelu(double x);

// Tests stress 100M+ neurones
bool neural_stress_test_100m_neurons(neural_config_t* config);

// Fonctions traçage neuronal complet
neural_activation_trace_t* neural_layer_trace_activations(neural_layer_t* layer);
bool neural_layer_save_gradients(neural_layer_t* layer, const char* filename);

// Configuration par défaut
neural_config_t* neural_config_create_default(void);
void neural_config_destroy(neural_config_t** config_ptr);
void neural_result_destroy(neural_result_t** result_ptr);
double neural_calculate_loss(double* predictions, double* targets, size_t size);
bool neural_validate_network_architecture(size_t* layer_sizes, size_t layer_count);

// Constantes
#define NEURAL_MAX_LAYERS 100
#define NEURAL_MAX_NEURONS_PER_LAYER 10000
#define NEURAL_MIN_LEARNING_RATE 1e-6
#define NEURAL_MAX_LEARNING_RATE 1.0
#define NEURAL_TRACE_MAGIC 0x4E455537
#define MAX_TRACE_LAYERS 32
#ifndef NEURAL_MAGIC_NUMBER
#define NEURAL_MAGIC_NUMBER 0x4E455552  // "NEUR" en ASCII
#define NEURAL_DESTROYED_MAGIC 0xDEADDEAD
#endif

// Alias de compatibilité pour le code existant
typedef activation_function_e activation_type_e;
#define NEURAL_DEFAULT_BIAS 0.0

#endif // NEURAL_NETWORK_PROCESSOR_H