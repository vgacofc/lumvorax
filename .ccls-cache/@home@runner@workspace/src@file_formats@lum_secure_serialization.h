
#ifndef LUM_SECURE_SERIALIZATION_H
#define LUM_SECURE_SERIALIZATION_H

#include "../lum/lum_core.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Constantes sérialisation sécurisée
#define LUM_SECURE_MAGIC 0x53454355  // "SECU" en ASCII
#define LUM_SECURE_VERSION 1
#define LUM_SECURE_MAX_CHUNK_SIZE 1048576  // 1MB par chunk

// Structure sérialisation sécurisée
typedef struct {
    uint32_t magic_number;        // Validation format
    uint32_t version;             // Version compatibilité  
    uint64_t timestamp;           // Horodatage création
    uint32_t checksum_crc32;      // Intégrité données
    uint32_t data_size;           // Taille données
    uint32_t lum_count;           // Nombre de LUMs
    bool is_encrypted;            // Chiffrement activé
    void* memory_address;         // Protection double-free
} lum_secure_header_t;

// Structure résultat sérialisation
typedef struct {
    uint8_t* serialized_data;     // Données sérialisées
    size_t data_size;             // Taille totale
    uint32_t checksum;            // Checksum calculé
    bool success;                 // Succès opération
    char error_message[256];      // Message d'erreur
    void* memory_address;         // Protection mémoire
} lum_secure_result_t;

// Fonctions principales sérialisation sécurisée
lum_secure_result_t* lum_secure_serialize_group(lum_group_t* group, bool encrypt);
lum_group_t* lum_secure_deserialize_group(const uint8_t* data, size_t data_size, bool decrypt);

// Fonctions I/O fichier sécurisé
bool lum_secure_save_to_file(const char* filename, lum_group_t* group, bool encrypt);
lum_group_t* lum_secure_load_from_file(const char* filename, bool decrypt);

// Fonctions validation intégrité
uint32_t lum_secure_calculate_checksum(const uint8_t* data, size_t size);
bool lum_secure_verify_integrity(const lum_secure_header_t* header, const uint8_t* data);

// Fonctions stress test 100M+ LUMs
bool lum_secure_stress_test_100m_serialization(void);

// Gestion mémoire résultats
void lum_secure_result_destroy(lum_secure_result_t** result_ptr);

#endif // LUM_SECURE_SERIALIZATION_H
