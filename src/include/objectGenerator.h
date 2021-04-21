//
// Created by Александр Дремов on 19.04.2021.
//

#ifndef MACHOBUILDER_OBJECTGENERATOR_H
#define MACHOBUILDER_OBJECTGENERATOR_H
#ifndef PUBLIC_HEADER
#include "HashMasm.h"
#include "binaryFile.h"
    #endif
#include <cstdio>
#include <MachOBuilder.h>

struct ObjectMachOGen{
    relocatePayload relPayload;
    MachoFileBin machoFile;
    const char* code;
    const char* data;
    size_t codeSize;
    size_t dataSize;

    void init();

    void dest();

    void addCode(const char* setCode, size_t size);

    void addData(const char* setCode, size_t size);

    void addData(const unsigned char* setData, size_t size);

    void addCode(const unsigned char* setData, size_t size);

    void dumpFile(binaryFile& binary);

    void bindSignedOffset(const char *name, size_t offsetBind);

    void bindBranchExt(const char* name, size_t offsetName);

    void generalSetup(loadCommand &codeSegment, segmentSection &codeSection);

    void addDataIfNeeded(loadCommand &codeSegment);

    void addInternalDataSymbol(const char *symbol, size_t offset);
    void addInternalCodeSymbol(const char *symbol, size_t offset);
};

#endif //MACHOBUILDER_OBJECTGENERATOR_H
