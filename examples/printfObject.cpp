//
// Created by Александр Дремов on 09.04.2021.
//
#include <MachOBuilder.h>
#include <cstdio>

int main() {
    FILE *res = fopen("machoObjectAuto.o", "wb");
    binaryFile binary = {};
    binary.init(res);

    ObjectMachOGen mgen = {};
    mgen.init();

    unsigned char asmCode[] = {
            0x55, 0x48, 0x89, 0xE5,
            0xE8, 0x00, 0x00, 0x00, 0x00, // call __Z8printTenv
            0xE8, 0x00, 0x00, 0x00, 0x00, // call __Z8printTenv
            0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, // mov eax, dword ptr [rip + offset globalVar ]
            0x31, 0xC0, 0x5D,
            0xE8, 0x00, 0x00, 0x00, 0x00,
            0xC3
    };

    unsigned char data[] = {
            0xDE, 0xD3, 0x2D, 0xED, 0x32, 0xDE, 0xD3, 0x2D, 0xED, 0x32,
            0xDE, 0xD3, 0x2D, 0xED, 0x32, 0xDE, 0xD3, 0x2D, 0xED, 0x32,
    };

    mgen.addCode(asmCode, sizeof(asmCode));
    mgen.addData(data, sizeof(data));

    mgen.addInternalCodeSymbol("_main", 0);
    mgen.addInternalDataSymbol("globalVar", 0);

    mgen.bindBranchExt("__Z5printi", 0x5);
    mgen.bindBranchExt("__Z5printi", 0xA);
    mgen.bindSignedOffset("globalVar", 16);

    mgen.dumpFile(binary);

    mgen.dest();
    binary.dest();
}