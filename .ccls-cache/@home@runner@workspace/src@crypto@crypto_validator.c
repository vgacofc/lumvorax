
#include "crypto_validator.h"
#include "sha256_test_vectors.h"
#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"  // NOUVEAU: Pour TRACKED_MALLOC/FREE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// SHA-256 constants (RFC 6234)
static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Forward declaration for secure_memcmp to fix compilation error
static int secure_memcmp(const void* a, const void* b, size_t len);

// SHA-256 functions
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

void sha256_init(sha256_context_t* ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
    ctx->buffer_length = 0;
}

void sha256_update(sha256_context_t* ctx, const uint8_t* data, size_t len) {
    size_t i = 0;
    
    while (i < len) {
        ctx->buffer[ctx->buffer_length] = data[i];
        ctx->buffer_length++;
        
        if (ctx->buffer_length == 64) {
            sha256_process_block(ctx, ctx->buffer);
            ctx->count += 512;
            ctx->buffer_length = 0;
        }
        i++;
    }
}

void sha256_final(sha256_context_t* ctx, uint8_t* hash) {
    uint64_t total_bits = ctx->count + (ctx->buffer_length * 8);
    
    // Padding
    ctx->buffer[ctx->buffer_length] = 0x80;
    ctx->buffer_length++;
    
    if (ctx->buffer_length > 56) {
        while (ctx->buffer_length < 64) {
            ctx->buffer[ctx->buffer_length] = 0x00;
            ctx->buffer_length++;
        }
        sha256_process_block(ctx, ctx->buffer);
        ctx->buffer_length = 0;
    }
    
    while (ctx->buffer_length < 56) {
        ctx->buffer[ctx->buffer_length] = 0x00;
        ctx->buffer_length++;
    }
    
    // Append length in bits as big-endian 64-bit
    for (int i = 0; i < 8; i++) {
        ctx->buffer[56 + i] = (total_bits >> (8 * (7 - i))) & 0xff;
    }
    
    sha256_process_block(ctx, ctx->buffer);
    
    // Output hash as big-endian
    for (int i = 0; i < 8; i++) {
        hash[i * 4] = (ctx->state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = ctx->state[i] & 0xff;
    }
    
    // SÉCURITÉ: Clearing sécurisé du contexte
    volatile uint8_t* vol_ctx = (volatile uint8_t*)ctx;
    for (size_t i = 0; i < sizeof(sha256_context_t); i++) {
        vol_ctx[i] = 0;
    }
}

void sha256_process_block(sha256_context_t* ctx, const uint8_t* block) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;
    
    // Prepare message schedule
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }
    
    // Initialize working variables
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    
    // Main loop
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + sha256_k[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    // Add compressed chunk to current hash value
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256_hash(const uint8_t* data, size_t len, uint8_t* hash) {
    sha256_context_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

bool crypto_validate_sha256_implementation(void) {
    bool all_tests_passed = true;
    
    // Tests RFC 6234 complets avec timing-safe comparison
    
    // Test vector 1: Empty string
    const char* input1 = "";
    const char* expected1 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    uint8_t result1[32];
    sha256_hash((const uint8_t*)input1, strlen(input1), result1);
    
    char hex1[65];
    bytes_to_hex_string(result1, 32, hex1);
    if (secure_memcmp(hex1, expected1, 64) != 0) all_tests_passed = false;
    
    // Test vector 2: "abc"
    const char* input2 = "abc";
    const char* expected2 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    uint8_t result2[32];
    sha256_hash((const uint8_t*)input2, strlen(input2), result2);
    
    char hex2[65];
    bytes_to_hex_string(result2, 32, hex2);
    if (secure_memcmp(hex2, expected2, 64) != 0) all_tests_passed = false;
    
    // Test vector 3: Long string
    const char* input3 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const char* expected3 = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";
    uint8_t result3[32];
    sha256_hash((const uint8_t*)input3, strlen(input3), result3);
    
    char hex3[65];
    bytes_to_hex_string(result3, 32, hex3);
    if (secure_memcmp(hex3, expected3, 64) != 0) all_tests_passed = false;
    
    // Test vector 4: Single "a" (nouveau test NIST)
    const char* input4 = "a";
    const char* expected4 = "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb";
    uint8_t result4[32];
    sha256_hash((const uint8_t*)input4, strlen(input4), result4);
    
    char hex4[65];
    bytes_to_hex_string(result4, 32, hex4);
    if (secure_memcmp(hex4, expected4, 64) != 0) all_tests_passed = false;
    
    // Test vector 5: Pattern de 56 bytes (boundary test)
    const char* input5 = "01234567012345670123456701234567012345670123456701234567";
    const char* expected5 = "b9045a713caed5dff3d3b783e98d1ce5778d8bc331ee4119d707072312af06a7";
    uint8_t result5[32];
    sha256_hash((const uint8_t*)input5, strlen(input5), result5);
    
    char hex5[65];
    bytes_to_hex_string(result5, 32, hex5);
    if (secure_memcmp(hex5, expected5, 64) != 0) all_tests_passed = false;
    
    return all_tests_passed;
}

void bytes_to_hex_string(const uint8_t* bytes, size_t len, char* hex_str) {
    for (size_t i = 0; i < len; i++) {
        snprintf(&hex_str[i * 2], 3, "%02x", bytes[i]);  // Secure: max 2 chars + null
    }
    hex_str[len * 2] = '\0';
}

bool hex_string_to_bytes(const char* hex_str, uint8_t* bytes, size_t max_len) {
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0 || hex_len / 2 > max_len) return false;
    
    for (size_t i = 0; i < hex_len / 2; i++) {
        int high = hex_char_to_int(hex_str[i * 2]);
        int low = hex_char_to_int(hex_str[i * 2 + 1]);
        if (high == -1 || low == -1) return false;
        bytes[i] = (high << 4) | low;
    }
    return true;
}

int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Hash calculator implementation
hash_calculator_t* hash_calculator_create(void) {
    hash_calculator_t* calc = TRACKED_MALLOC(sizeof(hash_calculator_t));
    if (!calc) return NULL;
    
    calc->algorithm = CRYPTO_ALGO_SHA256;
    calc->is_initialized = true;
    calc->total_operations = 0;
    calc->last_operation_time = time(NULL);
    
    return calc;
}

void hash_calculator_destroy(hash_calculator_t* calc) {
    if (calc) {
        TRACKED_FREE(calc);
    }
}

bool compute_data_hash(const void* data, size_t data_size, char* hash_output) {
    if (!data || !hash_output) return false;
    
    uint8_t hash[32];
    sha256_hash((const uint8_t*)data, data_size, hash);
    bytes_to_hex_string(hash, 32, hash_output);
    
    return true;
}

// Protection contre timing attacks - comparaison à temps constant
static int secure_memcmp(const void* a, const void* b, size_t len) {
    const volatile unsigned char* va = (const volatile unsigned char*)a;
    const volatile unsigned char* vb = (const volatile unsigned char*)b;
    unsigned char result = 0;
    
    for (size_t i = 0; i < len; i++) {
        result |= va[i] ^ vb[i];
    }
    
    return result;
}

bool verify_data_integrity(const void* data, size_t data_size, const char* expected_hash) {
    if (!data || !expected_hash) return false;
    
    char computed_hash[65];
    if (!compute_data_hash(data, data_size, computed_hash)) return false;
    
    // Utilisation comparaison timing-safe
    return secure_memcmp(computed_hash, expected_hash, strlen(expected_hash)) == 0;
}

bool validate_lum_integrity(const lum_t* lum) {
    if (!lum) return false;
    
    // Compute hash of LUM structure
    char hash_str[65];
    bool hash_ok = compute_data_hash(lum, sizeof(lum_t), hash_str);
    
    // Basic integrity checks
    if (!hash_ok) return false;
    if (lum->presence > 1) return false;
    if (lum->structure_type > LUM_STRUCTURE_MAX) return false;
    
    return true;
}

signature_result_t* generate_digital_signature(const void* data, size_t data_size) {
    signature_result_t* result = TRACKED_MALLOC(sizeof(signature_result_t));
    if (!result) return NULL;
    
    // Simple signature = hash + timestamp
    uint8_t hash[32];
    sha256_hash((const uint8_t*)data, data_size, hash);
    
    result->signature_length = 32;
    memcpy(result->signature_data, hash, 32);
    result->timestamp = time(NULL);
    result->is_valid = true;
    
    return result;
}

bool verify_digital_signature(const void* data, size_t data_size, const signature_result_t* signature) {
    if (!data || !signature) return false;
    
    uint8_t computed_hash[32];
    sha256_hash((const uint8_t*)data, data_size, computed_hash);
    
    return memcmp(computed_hash, signature->signature_data, 32) == 0;
}

void signature_result_destroy(signature_result_t* result) {
    if (result) {
        TRACKED_FREE(result);
    }
}
