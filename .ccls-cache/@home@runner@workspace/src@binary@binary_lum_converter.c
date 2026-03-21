#include "binary_lum_converter.h"
#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

// Fonction portable pour conversion 64-bit endianness
static uint64_t portable_hton64(uint64_t value) {
    static const int endian_test = 1;
    if (*(char*)&endian_test == 1) { // Little endian
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    }
    return value; // Big endian
}

static uint64_t portable_ntoh64(uint64_t value) {
    static const int endian_test = 1;
    if (*(char*)&endian_test == 1) { // Little endian
        return ((uint64_t)ntohl(value & 0xFFFFFFFF) << 32) | ntohl(value >> 32);
    }
    return value; // Big endian
}

// Core conversion: Binary data to LUM group
binary_lum_result_t* convert_binary_to_lum(const uint8_t* binary_data, size_t byte_count) {
    binary_lum_result_t* result = binary_lum_result_create();
    if (!result || !binary_data || byte_count == 0) {
        if (result) {
            result->success = false;
            SAFE_STRCPY(result->error_message, "Invalid input parameters", sizeof(result->error_message));
        }
        return result;
    }

    size_t total_bits = byte_count * 8;
    lum_group_t* lum_group = lum_group_create(total_bits);
    if (!lum_group) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Failed to create LUM group", sizeof(result->error_message));
        return result;
    }

    // Convert each bit to a LUM
    for (size_t byte_idx = 0; byte_idx < byte_count; byte_idx++) {
        uint8_t byte_val = binary_data[byte_idx];

        for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {  // MSB first
            uint8_t bit_val = (byte_val >> bit_idx) & 1;

            lum_t* lum = lum_create(bit_val, 
                                   (int32_t)(byte_idx * 8 + (7 - bit_idx)), 0, 
                                   LUM_STRUCTURE_LINEAR);
            if (lum) {
                lum_group_add(lum_group, lum);
                // NOTE: lum_group_add prend possession du LUM, pas de destroy nécessaire
            }
        }
    }

    result->lum_group = lum_group;
    result->success = true;
    result->bits_processed = total_bits;
    SAFE_STRCPY(result->error_message, "Conversion successful", sizeof(result->error_message));

    return result;
}

// Convert bit string to LUM group
binary_lum_result_t* convert_bits_to_lum(const char* bit_string) {
    binary_lum_result_t* result = binary_lum_result_create();
    if (!result || !bit_string) {
        if (result) {
            result->success = false;
            SAFE_STRCPY(result->error_message, "Invalid bit string", sizeof(result->error_message));
        }
        return result;
    }

    if (!validate_binary_string(bit_string)) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Invalid binary string format", sizeof(result->error_message));
        return result;
    }

    size_t bit_count = strlen(bit_string);
    lum_group_t* lum_group = lum_group_create(bit_count);
    if (!lum_group) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Failed to create LUM group", sizeof(result->error_message));
        return result;
    }

    // Convert each character to a LUM
    for (size_t i = 0; i < bit_count; i++) {
        uint8_t bit_val = (bit_string[i] == '1') ? 1 : 0;

        lum_t* lum = lum_create(bit_val, (int32_t)i, 0, LUM_STRUCTURE_LINEAR);
        if (lum) {
            lum_group_add(lum_group, lum);
            // NOTE: lum_group_add prend possession du LUM, pas de destroy nécessaire
        }
    }

    result->lum_group = lum_group;
    result->success = true;
    result->bits_processed = bit_count;
    SAFE_STRCPY(result->error_message, "Conversion successful", sizeof(result->error_message));

    return result;
}

// Convert LUM group to binary data
lum_binary_result_t* convert_lum_to_binary(const lum_group_t* lum_group) {
    lum_binary_result_t* result = lum_binary_result_create();
    if (!result || !lum_group || lum_group->count == 0) {
        if (result) {
            result->success = false;
            SAFE_STRCPY(result->error_message, "Invalid LUM group", sizeof(result->error_message));
        }
        return result;
    }

    size_t bit_count = lum_group->count;
    size_t byte_count = (bit_count + 7) / 8;  // Round up to nearest byte

    result->binary_data = TRACKED_CALLOC(byte_count, sizeof(uint8_t));
    if (!result->binary_data) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Memory allocation failed", sizeof(result->error_message));
        return result;
    }

    // Convert LUMs back to bits
    for (size_t i = 0; i < bit_count; i++) {
        if (i < lum_group->count) {
            uint8_t bit_val = lum_group->lums[i].presence;
            size_t byte_idx = i / 8;
            size_t bit_idx = 7 - (i % 8);  // MSB first

            if (bit_val) {
                result->binary_data[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    result->byte_count = byte_count;
    result->bit_count = bit_count;
    result->success = true;
    SAFE_STRCPY(result->error_message, "Conversion successful", sizeof(result->error_message));

    return result;
}

// Convert LUM group to bit string
lum_binary_result_t* convert_lum_to_bits(const lum_group_t* lum_group) {
    lum_binary_result_t* result = lum_binary_result_create();
    if (!result || !lum_group || lum_group->count == 0) {
        if (result) {
            result->success = false;
            SAFE_STRCPY(result->error_message, "Invalid LUM group", sizeof(result->error_message));
        }
        return result;
    }

    size_t bit_count = lum_group->count;

    result->binary_data = TRACKED_MALLOC(bit_count + 1);  // +1 for null terminator
    if (!result->binary_data) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Memory allocation failed", sizeof(result->error_message));
        return result;
    }

    // Convert each LUM to a character
    for (size_t i = 0; i < bit_count; i++) {
        result->binary_data[i] = lum_group->lums[i].presence ? '1' : '0';
    }
    result->binary_data[bit_count] = '\0';

    result->byte_count = bit_count + 1;
    result->bit_count = bit_count;
    result->success = true;
    SAFE_STRCPY(result->error_message, "Conversion successful", sizeof(result->error_message));

    return result;
}

// Integer conversion functions
binary_lum_result_t* convert_int8_to_lum(int8_t value) {
    uint8_t byte_val = (uint8_t)value;
    return convert_binary_to_lum(&byte_val, 1);
}

binary_lum_result_t* convert_int16_to_lum(int16_t value) {
    uint8_t bytes[2];
    bytes[0] = (uint8_t)(value >> 8);    // High byte
    bytes[1] = (uint8_t)(value & 0xFF);  // Low byte
    return convert_binary_to_lum(bytes, 2);
}

binary_lum_result_t* convert_int32_to_lum(int32_t value) {
    uint8_t bytes[4];
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)(value & 0xFF);
    return convert_binary_to_lum(bytes, 4);
}

binary_lum_result_t* convert_int64_to_lum(int64_t value) {
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = (uint8_t)(value >> (56 - i * 8));
    }
    // Utilisez la conversion portable pour s'assurer que l'ordre des octets est correct
    // regardless of the host system's endianness.
    uint64_t portable_value = portable_hton64(value);
    memcpy(bytes, &portable_value, sizeof(uint64_t));
    return convert_binary_to_lum(bytes, 8);
}

// LUM to integer conversion
int8_t convert_lum_to_int8(const lum_group_t* lum_group) {
    if (!lum_group || lum_group->count < 8) return 0;

    uint8_t result = 0;
    for (int i = 0; i < 8 && i < (int)lum_group->count; i++) {
        if (lum_group->lums[i].presence) {
            result |= (1u << (7 - i));
        }
    }
    return (int8_t)result;
}

int16_t convert_lum_to_int16(const lum_group_t* lum_group) {
    if (!lum_group || lum_group->count < 16) return 0;

    uint16_t result = 0;
    for (int i = 0; i < 16 && i < (int)lum_group->count; i++) {
        if (lum_group->lums[i].presence) {
            result |= (1u << (15 - i));
        }
    }
    return (int16_t)result;
}

int32_t convert_lum_to_int32(const lum_group_t* lum_group) {
    if (!lum_group || lum_group->count < 32) return 0;

    uint32_t result = 0;
    for (int i = 0; i < 32 && i < (int)lum_group->count; i++) {
        if (lum_group->lums[i].presence) {
            result |= (1u << (31 - i));
        }
    }
    return (int32_t)result;
}

int64_t convert_lum_to_int64(const lum_group_t* lum_group) {
    if (!lum_group || lum_group->count < 64) return 0;

    uint64_t result = 0;
    for (int i = 0; i < 64 && i < (int)lum_group->count; i++) {
        if (lum_group->lums[i].presence) {
            result |= (1LL << (63 - i));
        }
    }
    // Utilisez la conversion portable pour s'assurer que l'ordre des octets est correct
    // regardless of the host system's endianness.
    return portable_ntoh64(result);
}

// Float conversion functions
binary_lum_result_t* convert_float_to_lum(float value) {
    uint32_t int_val;
    memcpy(&int_val, &value, sizeof(uint32_t));  // Safe reinterpretation
    return convert_int32_to_lum((int32_t)int_val);
}

binary_lum_result_t* convert_double_to_lum(double value) {
    uint64_t int_val;
    memcpy(&int_val, &value, sizeof(uint64_t));  // Safe reinterpretation
    return convert_int64_to_lum((int64_t)int_val);
}

float convert_lum_to_float(const lum_group_t* lum_group) {
    int32_t int_val = convert_lum_to_int32(lum_group);
    float result;
    memcpy(&result, &int_val, sizeof(float));  // Safe reinterpretation
    return result;
}

double convert_lum_to_double(const lum_group_t* lum_group) {
    int64_t int_val = convert_lum_to_int64(lum_group);
    double result;
    memcpy(&result, &int_val, sizeof(double));  // Safe reinterpretation
    return result;
}

// Utility functions
binary_lum_result_t* binary_lum_result_create(void) {
    binary_lum_result_t* result = TRACKED_MALLOC(sizeof(binary_lum_result_t));
    if (result) {
        result->lum_group = NULL;
        result->success = false;
        result->error_message[0] = '\0';
        result->bits_processed = 0;
    }
    return result;
}

void binary_lum_result_destroy(binary_lum_result_t* result) {
    if (result) {
        if (result->lum_group) {
            lum_group_destroy(result->lum_group);
        }
        TRACKED_FREE(result);
    }
}

lum_binary_result_t* lum_binary_result_create(void) {
    lum_binary_result_t* result = TRACKED_MALLOC(sizeof(lum_binary_result_t));
    if (result) {
        result->binary_data = NULL;
        result->byte_count = 0;
        result->bit_count = 0;
        result->success = false;
        result->error_message[0] = '\0';
    }
    return result;
}

void lum_binary_result_destroy(lum_binary_result_t* result) {
    if (result) {
        if (result->binary_data) {
            TRACKED_FREE(result->binary_data);
        }
        TRACKED_FREE(result);
    }
}

// String representation functions
char* lum_group_to_binary_string(const lum_group_t* lum_group) {
    if (!lum_group || lum_group->count == 0) return NULL;

    char* result = TRACKED_MALLOC(lum_group->count + 1);
    if (!result) return NULL;

    for (size_t i = 0; i < lum_group->count; i++) {
        result[i] = lum_group->lums[i].presence ? '1' : '0';
    }
    result[lum_group->count] = '\0';

    return result;
}

char* lum_group_to_hex_string(const lum_group_t* lum_group) {
    if (!lum_group || lum_group->count == 0) return NULL;

    lum_binary_result_t* binary_result = convert_lum_to_binary(lum_group);
    if (!binary_result || !binary_result->success) {
        if (binary_result) lum_binary_result_destroy(binary_result);
        return NULL;
    }

    size_t hex_len = binary_result->byte_count * 2 + 1;
    char* hex_string = TRACKED_MALLOC(hex_len);
    if (!hex_string) {
        lum_binary_result_destroy(binary_result);
        return NULL;
    }

    for (size_t i = 0; i < binary_result->byte_count; i++) {
        snprintf(&hex_string[i * 2], 3, "%02X", binary_result->binary_data[i]);
    }

    lum_binary_result_destroy(binary_result);
    return hex_string;
}

// Validation functions
bool validate_binary_string(const char* bit_string) {
    if (!bit_string) return false;

    for (size_t i = 0; bit_string[i]; i++) {
        if (bit_string[i] != '0' && bit_string[i] != '1') {
            return false;
        }
    }
    return true;
}

bool validate_hex_string(const char* hex_string) {
    if (!hex_string) return false;

    for (size_t i = 0; hex_string[i]; i++) {
        char c = hex_string[i];
        if (!((c >= '0' && c <= '9') || 
              (c >= 'A' && c <= 'F') || 
              (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }
    return true;
}

size_t count_bits_in_string(const char* bit_string) {
    return bit_string ? strlen(bit_string) : 0;
}

// Endianness handling
binary_lum_result_t* convert_binary_to_lum_little_endian(const uint8_t* binary_data, size_t byte_count) {
    if (!binary_data || byte_count == 0) return NULL;

    // Reverse the byte order
    uint8_t* reversed_data = TRACKED_MALLOC(byte_count);
    if (!reversed_data) return NULL;

    for (size_t i = 0; i < byte_count; i++) {
        reversed_data[i] = binary_data[byte_count - 1 - i];
    }

    binary_lum_result_t* result = convert_binary_to_lum(reversed_data, byte_count);
    TRACKED_FREE(reversed_data);

    return result;
}

binary_lum_result_t* convert_binary_to_lum_big_endian(const uint8_t* binary_data, size_t byte_count) {
    // Big endian is our default format
    return convert_binary_to_lum(binary_data, byte_count);
}

binary_lum_result_t* convert_hex_string_to_lum(const char* hex_string) {
    binary_lum_result_t* result = binary_lum_result_create();
    if (!result || !hex_string) {
        if (result) {
            result->success = false;
            SAFE_STRCPY(result->error_message, "Invalid hex string", sizeof(result->error_message));
        }
        return result;
    }

    if (!validate_hex_string(hex_string)) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Invalid hex string format", sizeof(result->error_message));
        return result;
    }

    size_t hex_len = strlen(hex_string);
    if (hex_len % 2 != 0) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Hex string must have even length", sizeof(result->error_message));
        return result;
    }

    size_t byte_count = hex_len / 2;
    uint8_t* binary_data = TRACKED_MALLOC(byte_count);
    if (!binary_data) {
        result->success = false;
        SAFE_STRCPY(result->error_message, "Memory allocation failed", sizeof(result->error_message));
        return result;
    }

    // Convert hex pairs to bytes
    for (size_t i = 0; i < byte_count; i++) {
        char hex_pair[3] = {hex_string[i * 2], hex_string[i * 2 + 1], '\0'};
        binary_data[i] = (uint8_t)strtol(hex_pair, NULL, 16);
    }

    binary_lum_result_t* lum_result = convert_binary_to_lum(binary_data, byte_count);
    TRACKED_FREE(binary_data);

    if (lum_result) {
        result->lum_group = lum_result->lum_group;
        result->success = lum_result->success;
        result->bits_processed = lum_result->bits_processed;
        SAFE_STRCPY(result->error_message, lum_result->error_message, sizeof(result->error_message));

        // Transfer ownership, don't destroy the lum_group
        lum_result->lum_group = NULL;
        binary_lum_result_destroy(lum_result);
    }

    return result;
}