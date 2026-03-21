#ifndef SAFE_STRING_H
#define SAFE_STRING_H

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// Remplacements fonctions unsafe par versions sécurisées
#define SAFE_STRCPY(dest, src, dest_size) safe_strcpy_impl(dest, src, dest_size, __FILE__, __LINE__)
#define SAFE_STRCAT(dest, src, dest_size) safe_strcat_impl(dest, src, dest_size, __FILE__, __LINE__)

// Fonction strcpy sécurisée avec validation bounds
static inline bool safe_strcpy_impl(char* dest, const char* src, size_t dest_size, 
                     const char* file, int line) {
    if (!dest || !src || dest_size == 0) {
        fprintf(stderr, "[SECURITY_ERROR] safe_strcpy: Invalid parameters at %s:%d\n", file, line);
        return false;
    }

    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        fprintf(stderr, "[SECURITY_ERROR] safe_strcpy: Buffer overflow prevented at %s:%d "
                       "(src_len=%zu, dest_size=%zu)\n", file, line, src_len, dest_size);
        return false;
    }

    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    return true;
}

// Fonction strcat sécurisée
static inline bool safe_strcat_impl(char* dest, const char* src, size_t dest_size, 
                     const char* file, int line) {
    if (!dest || !src || dest_size == 0) {
        fprintf(stderr, "[SECURITY_ERROR] safe_strcat: Invalid parameters at %s:%d\n", file, line);
        return false;
    }

    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    if (dest_len + src_len >= dest_size) {
        fprintf(stderr, "[SECURITY_ERROR] safe_strcat: Buffer overflow prevented at %s:%d\n", file, line);
        return false;
    }

    memcpy(dest + dest_len, src, src_len);
    dest[dest_len + src_len] = '\0';
    return true;
}

#endif // SAFE_STRING_H