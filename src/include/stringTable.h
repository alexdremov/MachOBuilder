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

    unsigned addString(char *key);

    void dest();

    char** binWrite(binaryFile *out);
};

struct symbolTableEntry{
    nlist_64 list;
    size_t offset;
    bool external;
};

struct symbolTable {
    HashMasm<symbolTableEntry> storage;
    stringTablePayload payload;
    size_t offset;

    void init();

    void dest();

    void addExternal(char *key);

    void addInside(char *key, unsigned section);

    void binWrite(binaryFile *out);

    void writePayload(binaryFile *out);
};

#endif //MACHOBUILDER_STRINGTABLE_H
