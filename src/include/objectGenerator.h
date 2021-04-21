//
// Created by Александр Дремов on 19.04.2021.
//

#ifndef MACHOBUILDER_OBJECTGENERATOR_H
#define MACHOBUILDER_OBJECTGENERATOR_H
#include "HashMasm.h"
#include "binaryFile.h"
#include <cstdio>
#include <MachOBuilder.h>

struct ObjectMachOGen{
    relocatePayload relPayload;
    MachoFileBin machoFile;
    const char* code;
    const char* data;
    size_t codeSize;
    size_t dataSize;
    size_t mainOffset;

    void init();

    void dest();

    void bindBranchExt(const char* name, size_t offsetName);

    void addCode(const char* setCode, size_t size);

    void addData(const char* setCode, size_t size);

    void addData(const unsigned char* setData, size_t size);

    void addCode(const unsigned char* setData, size_t size);

    void setMain(size_t offset);

    void dumpFile(binaryFile& binary);

    void bindSignedOffsetData(const char *name, size_t offsetData, size_t offsetBind);

    void generalSetup(loadCommand &codeSegment, segmentSection &codeSection);

    void addDataIfNeeded(loadCommand &codeSegment);
};

#endif //MACHOBUILDER_OBJECTGENERATOR_H
