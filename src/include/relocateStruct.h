//
// Created by Александр Дремов on 17.04.2021.
//

#ifndef MACHOBUILDER_RELOCATESTRUCT_H
#define MACHOBUILDER_RELOCATESTRUCT_H
#ifndef PUBLIC_HEADER

#include <mach-o/reloc.h>
#include <binaryFile.h>

#endif

#include "FastList.h"

struct relocatePayload {
    FastList<relocation_info> info;

    void binWrite(binaryFile *out);

    binPayload bufferWrite();

    void addReloc(int32_t r_address, uint32_t r_symbolnum, uint32_t r_pcrel, uint32_t r_length,
                  uint32_t r_extern, uint32_t r_type);

    void addReloc(const relocation_info &other);

    void init();

    void dest();
};


#endif //MACHOBUILDER_RELOCATESTRUCT_H
