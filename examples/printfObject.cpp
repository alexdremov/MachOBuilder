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
            0xE8, 0x00, 0x00, 0x00, 0x00,
            0xE8, 0x00, 0x00, 0x00, 0x00,
            0x31, 0xC0, 0x5D,  0xC3
    };

    unsigned char data[] = {
            0xDE, 0xD0
    };

    mgen.addCode(asmCode, sizeof(asmCode));
    mgen.setMain(0);
    mgen.bind("__Z8printTenv", 0x5);
    mgen.bind("__Z8printTenv", 0xA);
    mgen.bindVarData("globalVar", 0, 1);

    mgen.addData(data, sizeof(data));
    mgen.dumpFile(binary);

    mgen.dest();
    binary.dest();
}