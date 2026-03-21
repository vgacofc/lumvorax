
#ifndef SHA256_TEST_VECTORS_H
#define SHA256_TEST_VECTORS_H

// Vecteurs de test officiels RFC 6234 pour SHA-256
typedef struct {
    const char* input;
    const char* expected_hash;
    size_t input_length;
} sha256_test_vector_t;

static const sha256_test_vector_t RFC6234_VECTORS[] = {
    // Vecteur 1: chaîne vide
    {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 0},
    
    // Vecteur 2: "abc"
    {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 3},
    
    // Vecteur 3: chaîne longue
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 
     "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", 56},
    
    // Vecteur 4: 1 million de 'a' (test de performance)
    {NULL, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", 1000000},
    
    // Terminateur
    {NULL, NULL, 0}
};

#define RFC6234_VECTOR_COUNT 4

#endif // SHA256_TEST_VECTORS_H
