//
// Created by Александр Дремов on 06.04.2021.
//

#ifndef HASHMASM_HASHES_H
#define HASHMASM_HASHES_H
#include <cstdlib>

namespace CRC {
    size_t hash64(size_t crc, const unsigned char *p);
    size_t hash32(size_t crc, const unsigned char *p);
    extern "C" size_t hash32AsmFLen(const unsigned char *p, size_t len, size_t hash = 0);
    extern "C" size_t hash32Asm(const unsigned char *p, size_t len, size_t hash = 0);
}

namespace FNV {
    size_t fnv64(const char *p, size_t hash = 14695981039346656037ULL);
    extern "C" size_t fnv64Asm(const char *p, size_t hash = 14695981039346656037ULL);
}

namespace PolyHash {
    size_t poly64(const char *p, size_t hash = 0);
}

#endif //HASHMASM_HASHES_H
