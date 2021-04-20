//
// Created by Александр Дремов on 17.04.2021.
//

#ifndef MACHOBUILDER_STRINGTABLE_H
#define MACHOBUILDER_STRINGTABLE_H

#include <FastList.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include "HashMasm.h"
#include "binaryFile.h"

struct stringIndOffset {
    size_t index;
    size_t offset;
};

struct stringTablePayload {
    HashMasm<stringIndOffset> storage;
    size_t offset;
    size_t size;

    void init();

    unsigned addString(const char *key);

    void dest();

    char** binWrite(binaryFile *out);
};

struct symbolTableEntry{
    enum symbolTableType {
        SYM_TYPE_EXTERNAL,
        SYM_TYPE_INTERNAL
    };
    nlist_64 list;
    size_t offset;
    symbolTableType type;
};

struct symbolTable {
    HashMasm<symbolTableEntry> storage;
    stringTablePayload payload;
    size_t offset;

    void init();

    void dest();

    void addExternal(const char *key);

    void binWrite(binaryFile *out);

    void writePayload(binaryFile *out);

    void addInternal(const char *key, unsigned int section, size_t offset = 0);

    void addData(const char *key, unsigned int section, size_t offset = 0);
};

#endif //MACHOBUILDER_STRINGTABLE_H
