#ifndef QUANTUM_SIMULATOR_H
#define QUANTUM_SIMULATOR_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <complex.h>

// Module Simulateur Quantique pour LUM/VORAX
// Conforme prompt.txt - nouveau module calculs avancés

// LUM Quantique avec superposition et intrication
typedef struct {
    lum_t base_lum;                // LUM de base
    double complex* amplitudes;   // Amplitudes quantiques (superposition)
    size_t state_count;           // Nombre d'états superposés
    uint64_t* entangled_ids;      // IDs des LUMs intriqués
    size_t entanglement_count;    // Nombre d'intrications
    double coherence_time;        // Temps de cohérence (ns)
    double fidelity;              // Fidélité quantique [0,1]
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint32_t quantum_magic;       // Validation intégrité quantique
    bool is_measured;             // État mesuré (collapse)
} quantum_lum_t;

// Portes quantiques implémentées
typedef enum {
    QUANTUM_GATE_HADAMARD = 0,    // Porte Hadamard (superposition)
    QUANTUM_GATE_PAULI_X = 1,     // Porte Pauli-X (NOT quantique)
    QUANTUM_GATE_PAULI_Y = 2,     // Porte Pauli-Y
    QUANTUM_GATE_PAULI_Z = 3,     // Porte Pauli-Z
    QUANTUM_GATE_CNOT = 4,        // Porte CNOT (intrication)
    QUANTUM_GATE_PHASE = 5,       // Porte de phase
    QUANTUM_GATE_T = 6,           // Porte T (π/8)
    QUANTUM_GATE_S = 7,           // Porte S (π/4)
    QUANTUM_GATE_TOFFOLI = 8      // Porte Toffoli (3-qubits)
} quantum_gate_e;

// Circuit quantique
typedef struct {
    quantum_lum_t** qubits;       // Array de qubits quantiques
    size_t qubit_count;           // Nombre de qubits
    quantum_gate_e* gate_sequence;// Séquence de portes
    size_t gate_count;            // Nombre de portes
    size_t* gate_targets;         // Qubits cibles pour chaque porte
    size_t* gate_controls;        // Qubits de contrôle
    double total_coherence;       // Cohérence totale du circuit
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint64_t execution_time_ns;   // Temps d'exécution nanoseconde
} quantum_circuit_t;

// Résultat simulation quantique
typedef struct {
    quantum_lum_t** final_states; // États finaux des qubits
    size_t state_count;           // Nombre d'états finaux
    double* probabilities;        // Probabilités de chaque état
    double complex* state_vector; // Vecteur d'état complet
    size_t vector_size;           // Taille du vecteur d'état
    double fidelity_loss;         // Perte de fidélité durant simulation
    bool success;                 // Succès simulation
    char error_message[256];      // Message d'erreur
    uint64_t quantum_operations;  // Nombre d'opérations quantiques
    void* memory_address;         // Protection double-free OBLIGATOIRE
} quantum_result_t;

// Configuration simulateur quantique
typedef struct {
    double decoherence_rate;      // Taux de décohérence (1/ns)
    double gate_error_rate;       // Taux d'erreur des portes [0,1]
    bool enable_noise_model;      // Modèle de bruit quantique
    size_t max_entanglement;      // Intrication maximale
    bool use_gpu_acceleration;    // Accélération GPU
    double temperature_kelvin;    // Température du système (mK)
    void* memory_address;         // Protection double-free OBLIGATOIRE
} quantum_config_t;

// Simulateur quantique complet
typedef struct {
    size_t qubit_count;           // Nombre de qubits
    size_t max_gates;             // Nombre maximum de portes
    size_t state_vector_size;     // Taille du vecteur d'état
    quantum_circuit_t* circuit;  // Circuit quantique associé
    quantum_config_t* config;    // Configuration du simulateur
    double* state_probabilities;  // Probabilités des états
    bool is_initialized;          // État d'initialisation
    void* memory_address;         // Protection double-free OBLIGATOIRE
    uint32_t magic_number;        // Protection intégrité
} quantum_simulator_t;

// Fonctions principales
quantum_lum_t* quantum_lum_create(int32_t x, int32_t y, size_t initial_states);
void quantum_lum_destroy(quantum_lum_t** qlum_ptr);
quantum_circuit_t* quantum_circuit_create(size_t qubit_count, size_t max_gates);
void quantum_circuit_destroy(quantum_circuit_t** circuit_ptr);

// Fonctions simulateur quantique
quantum_simulator_t* quantum_simulator_create(size_t qubit_count, quantum_config_t* config);
void quantum_simulator_destroy(quantum_simulator_t** simulator_ptr);

// Opérations quantiques de base
bool quantum_apply_gate(quantum_lum_t* qlum, quantum_gate_e gate, quantum_config_t* config);
bool quantum_entangle_lums(quantum_lum_t* qlum1, quantum_lum_t* qlum2, quantum_config_t* config);
quantum_result_t* quantum_measure(quantum_lum_t* qlum, quantum_config_t* config);
bool quantum_create_superposition(quantum_lum_t* qlum, double* amplitudes, size_t state_count);

// Simulation de circuits
quantum_result_t* quantum_simulate_circuit(quantum_circuit_t* circuit, quantum_config_t* config);
bool quantum_add_gate_to_circuit(quantum_circuit_t* circuit, quantum_gate_e gate, size_t target, size_t control);

// Algorithmes quantiques avancés
quantum_result_t* quantum_shor_algorithm(uint64_t number_to_factor, quantum_config_t* config);
quantum_result_t* quantum_grover_search(lum_group_t* search_space, lum_t* target, quantum_config_t* config);
quantum_result_t* quantum_quantum_fourier_transform(quantum_circuit_t* circuit, quantum_config_t* config);

// Tests stress 100M+ LUMs quantiques
bool quantum_stress_test_100m_qubits(quantum_config_t* config);
quantum_result_t* quantum_benchmark_entanglement(size_t qubit_count, quantum_config_t* config);
quantum_result_t* quantum_test_decoherence_scaling(size_t max_qubits, quantum_config_t* config);

// Utilitaires
quantum_config_t* quantum_config_create_default(void);
void quantum_config_destroy(quantum_config_t** config_ptr);
void quantum_result_destroy(quantum_result_t** result_ptr);
double quantum_calculate_fidelity(quantum_lum_t* qlum_ideal, quantum_lum_t* qlum_actual);
bool quantum_validate_state_vector(double complex* state_vector, size_t size);

// Constantes quantiques
#define QUANTUM_MAX_QUBITS 64
#define QUANTUM_MIN_FIDELITY 0.95
#define QUANTUM_PLANCK_CONSTANT 6.62607015e-34
#define QUANTUM_MAGIC_NUMBER 0x51554E54
#define QUANTUM_DESTROYED_MAGIC 0xDEADBEEF

#endif // QUANTUM_SIMULATOR_H