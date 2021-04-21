//
// Created by Александр Дремов on 17.04.2021.
//

#ifndef MACHOBUILDER_RELOCATESTRUCT_H
#define MACHOBUILDER_RELOCATESTRUCT_H

#include <mach-o/reloc.h>
#ifndef PUBLIC_HEADER
#include "binaryFile.h"
    #endif


#include <FastList.h>

struct relocationInfo{
    relocation_info info;
    const char* name;
};

struct relocatePayload {
    FastList<relocationInfo> info;

    void binWrite(binaryFile *out);

    binPayload bufferWrite();

    void addReloc(const char* name, int32_t r_address, uint32_t r_symbolnum, uint32_t r_pcrel, uint32_t r_length,
                  uint32_t r_extern, uint32_t r_type);

    void addReloc(const relocationInfo &other);

    void init();

    void dest();
};


#endif //MACHOBUILDER_RELOCATESTRUCT_H
