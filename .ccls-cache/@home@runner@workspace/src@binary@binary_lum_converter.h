#ifndef BINARY_LUM_CONVERTER_H
#define BINARY_LUM_CONVERTER_H

#include "../lum/lum_core.h"
#include <stdint.h>

// Binary to LUM conversion result
typedef struct {
    lum_group_t* lum_group;
    bool success;
    char error_message[256];
    size_t bits_processed;
} binary_lum_result_t;

// LUM to Binary conversion result
typedef struct {
    uint8_t* binary_data;
    size_t byte_count;
    size_t bit_count;
    bool success;
    char error_message[256];
} lum_binary_result_t;

// Core conversion functions
binary_lum_result_t* convert_binary_to_lum(const uint8_t* binary_data, size_t byte_count);
binary_lum_result_t* convert_bits_to_lum(const char* bit_string);
lum_binary_result_t* convert_lum_to_binary(const lum_group_t* lum_group);
lum_binary_result_t* convert_lum_to_bits(const lum_group_t* lum_group);

// Integer conversion functions
binary_lum_result_t* convert_int8_to_lum(int8_t value);
binary_lum_result_t* convert_int16_to_lum(int16_t value);
binary_lum_result_t* convert_int32_to_lum(int32_t value);
binary_lum_result_t* convert_int64_to_lum(int64_t value);

// LUM to integer conversion
int8_t convert_lum_to_int8(const lum_group_t* lum_group);
int16_t convert_lum_to_int16(const lum_group_t* lum_group);
int32_t convert_lum_to_int32(const lum_group_t* lum_group);
int64_t convert_lum_to_int64(const lum_group_t* lum_group);

// Utility functions
binary_lum_result_t* binary_lum_result_create(void);
void binary_lum_result_destroy(binary_lum_result_t* result);
lum_binary_result_t* lum_binary_result_create(void);
void lum_binary_result_destroy(lum_binary_result_t* result);

// String representation functions
char* lum_group_to_binary_string(const lum_group_t* lum_group);
char* lum_group_to_hex_string(const lum_group_t* lum_group);
binary_lum_result_t* convert_hex_string_to_lum(const char* hex_string);

// Validation functions
bool validate_binary_string(const char* bit_string);
bool validate_hex_string(const char* hex_string);
size_t count_bits_in_string(const char* bit_string);

// Advanced conversion functions
binary_lum_result_t* convert_float_to_lum(float value);
binary_lum_result_t* convert_double_to_lum(double value);
float convert_lum_to_float(const lum_group_t* lum_group);
double convert_lum_to_double(const lum_group_t* lum_group);

// Endianness handling
binary_lum_result_t* convert_binary_to_lum_little_endian(const uint8_t* binary_data, size_t byte_count);
binary_lum_result_t* convert_binary_to_lum_big_endian(const uint8_t* binary_data, size_t byte_count);

#endif // BINARY_LUM_CONVERTER_H