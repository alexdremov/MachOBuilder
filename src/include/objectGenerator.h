//
// Created by Александр Дремов on 19.04.2021.
//

#ifndef MACHOBUILDER_OBJECTGENERATOR_H
#define MACHOBUILDER_OBJECTGENERATOR_H
#include "HashMasm.h"
#include "MachOBuilder.h"
#include "binaryFile.h"
#include <cstdio>

struct ObjectMachOGen{
    HashMasm<FastList<size_t>> offsets;
    const char* code;
    const char* data;
    size_t codeSize;
    size_t dataSize;
    size_t mainOffset;

    void init();

    void dest();

    void bind(const char* name, size_t offset);

    void addCode(const char* setCode, size_t size);

    void addData(const char* setCode, size_t size);

    void addData(const unsigned char* setData, size_t size);

    void addCode(const unsigned char* setData, size_t size);

    void setMain(size_t offset);

    void dumpFile(binaryFile& binary);
};

#endif //MACHOBUILDER_OBJECTGENERATOR_H
