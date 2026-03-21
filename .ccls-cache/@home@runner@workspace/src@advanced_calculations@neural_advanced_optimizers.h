// NOUVEAU FICHIER : src/advanced_calculations/neural_advanced_optimizers.h

#ifndef NEURAL_ADVANCED_OPTIMIZERS_H
#define NEURAL_ADVANCED_OPTIMIZERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Types désactivés - placeholders
typedef struct {} neural_blackbox_computer_t;

// #include "neural_blackbox_computer.h" // DÉSACTIVÉ par prompt.txt

// OPTIMISEUR ADAM ULTRA-PRÉCIS
typedef struct {
    double* moment1;          // Premier moment (moyenne gradient)
    double* moment2;          // Second moment (variance gradient)  
    double beta1;             // Paramètre décroissance moment1 (0.9)
    double beta2;             // Paramètre décroissance moment2 (0.999)
    double epsilon;           // Terme régularisation (1e-12 pour précision)
    double learning_rate;     // Taux apprentissage adaptatif
    uint64_t step_count;      // Compteur étapes pour correction bias
    
    // INNOVATION : Décroissance adaptative selon convergence
    double convergence_factor; // Facteur réduction LR selon convergence
    double min_learning_rate;  // LR minimum (1e-8)
    double precision_threshold; // Seuil précision pour décroissance LR
    
    // Gestion mémoire et intégrité
    size_t parameter_count;   // Nombre paramètres
    void* memory_address;     // Protection double-free
    uint32_t magic_number;    // Validation intégrité
} neural_adam_ultra_precise_t;

// OPTIMISEUR L-BFGS POUR CONVERGENCE GARANTIE
typedef struct {
    double** s_vectors;       // Vecteurs s (changements paramètres)
    double** y_vectors;       // Vecteurs y (changements gradients)
    double* alpha;            // Coefficients pour récursion
    double* rho;              // Facteurs normalisation
    size_t memory_size;       // Taille mémoire L-BFGS (ex: 20)
    size_t current_position;  // Position actuelle dans mémoire circulaire
    bool memory_full;         // Mémoire L-BFGS pleine
    size_t parameter_count;   // Nombre paramètres
    
    // État convergence
    double last_loss;         // Dernière valeur loss
    double convergence_rate;  // Taux convergence
    uint64_t iterations;      // Nombre itérations
    
    // Gestion mémoire et intégrité  
    void* memory_address;     // Protection double-free
    uint32_t magic_number;    // Validation intégrité
} neural_lbfgs_optimizer_t;

// OPTIMISEUR NEWTON-RAPHSON ULTRA-PRÉCIS
typedef struct {
    double* hessian_matrix;    // Matrice Hessienne
    double* hessian_inverse;   // Inverse Hessienne
    double regularization;     // Terme régularisation
    size_t parameter_count;    // Nombre paramètres
    bool use_pseudo_inverse;   // Utiliser pseudo-inverse si singulière
    
    // Métriques précision
    double condition_number;   // Nombre condition Hessienne
    double determinant;        // Déterminant Hessienne
    
    // Gestion mémoire et intégrité
    void* memory_address;      // Protection double-free  
    uint32_t magic_number;     // Validation intégrité
} neural_newton_raphson_t;

// FONCTIONS OPTIMISEURS AVANCÉS

// === OPTIMISEUR ADAM ULTRA-PRÉCIS ===
neural_adam_ultra_precise_t* neural_adam_create_ultra_precise(size_t parameter_count);
void neural_adam_destroy_ultra_precise(neural_adam_ultra_precise_t** optimizer);
void neural_adam_ultra_precise_update(
    void* system, // neural_blackbox_computer_t désactivé
    neural_adam_ultra_precise_t* optimizer,
    double* gradients,
    double current_loss
);
bool neural_adam_reset_moments(neural_adam_ultra_precise_t* optimizer);

// === OPTIMISEUR L-BFGS ===
neural_lbfgs_optimizer_t* neural_lbfgs_create(size_t parameter_count, size_t memory_size);
void neural_lbfgs_destroy(neural_lbfgs_optimizer_t** optimizer);
bool neural_lbfgs_ultra_precise_step(
    neural_blackbox_computer_t* system,
    neural_lbfgs_optimizer_t* lbfgs,
    double* current_gradient,
    double current_loss
);
bool neural_lbfgs_update_history(
    neural_lbfgs_optimizer_t* lbfgs,
    double* parameter_change,
    double* gradient_change
);

// === OPTIMISEUR NEWTON-RAPHSON ===
neural_newton_raphson_t* neural_newton_raphson_create(size_t parameter_count);
void neural_newton_raphson_destroy(neural_newton_raphson_t** optimizer);
void neural_newton_raphson_ultra_precise_step(
    neural_blackbox_computer_t* system,
    neural_newton_raphson_t* optimizer,
    double* gradients,
    double* hessian
);
bool neural_newton_raphson_compute_hessian_inverse(
    neural_newton_raphson_t* optimizer,
    double* hessian
);

// === LINE SEARCH AVEC CONDITIONS WOLFE ===
double neural_wolfe_line_search_ultra_precise(
    void* system, // neural_blackbox_computer_t désactivé
    double* search_direction,
    double* gradient,
    double current_loss
);

// === FONCTIONS UTILITAIRES ===

// Calcul norme euclidienne haute précision
double neural_vector_norm_ultra_precise(double* vector, size_t size);

// Produit scalaire haute précision
double neural_dot_product_ultra_precise(double* v1, double* v2, size_t size);

// Copie vecteur sécurisée
void neural_vector_copy(double* dest, double* src, size_t size);

// Calcul déterminant matrice (pour Newton-Raphson)
double neural_matrix_determinant(double* matrix, size_t size);

// Inversion matrice avec régularisation
bool neural_matrix_inverse_regularized(
    double* matrix, 
    double* inverse, 
    size_t size, 
    double regularization
);

// Validation gradient pour détection NaN/Inf
bool neural_validate_gradient(double* gradient, size_t size);

#endif // NEURAL_ADVANCED_OPTIMIZERS_H