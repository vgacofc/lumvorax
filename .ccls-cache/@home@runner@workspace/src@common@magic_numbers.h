#ifndef MAGIC_NUMBERS_H
#define MAGIC_NUMBERS_H

#include <stdint.h>

// Magic numbers unifiés système LUM/VORAX - CORRECTION RAPPORT 129 ANOMALIE #001
// Utilisation de codes ASCII pour identification claire

// Magic numbers structures principales  
#define LUM_CORE_MAGIC           0x4C554D43  // "LUMC" - LUM Core
#define LUM_GROUP_MAGIC          0x4C554D47  // "LUMG" - LUM Group
#define LUM_ZONE_MAGIC           0x4C554D5A  // "LUMZ" - LUM Zone
#define VORAX_RESULT_MAGIC       0x564F5258  // "VORX" - VORAX Result
#define LUM_DISPLACEMENT_MAGIC   0x4C554D44  // "LUMD" - LUM Displacement

// Magic numbers modules spécialisés
#define MATRIX_CALCULATOR_MAGIC  0x4D415452  // "MATR" - Matrix
#define CRYPTO_VALIDATOR_MAGIC   0x43525950  // "CRYP" - Crypto
#define MEMORY_TRACKER_MAGIC     0x4D454D54  // "MEMT" - Memory Tracker
#define FORENSIC_LOGGER_MAGIC    0x464F524C  // "FORL" - Forensic Logger

// Magic numbers états unifiés (plus de collision 0xDEADBEEF)
#define MAGIC_DESTROYED_PATTERN  0xDEADC0DE  // Pattern unique destruction
#define MAGIC_CORRUPTED_PATTERN  0xBADC0FFE  // Pattern détection corruption
#define MAGIC_INITIALIZED        0x494E4954  // "INIT" - Initialisé
#define MAGIC_UNINITIALIZED      0x554E494E  // "UNIN" - Non initialisé

// Macros validation unifiées
#define VALIDATE_MAGIC(ptr, expected) \
    ((ptr) && ((ptr)->magic_number == (expected)))

#define MARK_DESTROYED(ptr, magic_field) \
    do { if (ptr) (ptr)->magic_field = MAGIC_DESTROYED_PATTERN; } while(0)

#define MARK_INITIALIZED(ptr, magic_field, expected_magic) \
    do { if (ptr) (ptr)->magic_field = (expected_magic); } while(0)

#define IS_DESTROYED(ptr, magic_field) \
    ((ptr) && ((ptr)->magic_field == MAGIC_DESTROYED_PATTERN))

#define IS_CORRUPTED(ptr, magic_field, expected_magic) \
    ((ptr) && ((ptr)->magic_field != (expected_magic)) && \
     ((ptr)->magic_field != MAGIC_DESTROYED_PATTERN))

#endif // MAGIC_NUMBERS_H