//
// Created by Александр Дремов on 09.04.2021.
//
#include <MachOBuilder.h>
#include <cstdio>


int main() {
    FILE *res = fopen("machoRetZeroApp", "wb");
    binaryFile binary = {};
    binary.init(res);

    unsigned char asmCode[] = {
            0xCD, 0x03, 0x6A, 0x03,
            0x48, 0x89, 0xC7, 0xB8, 0x01,
            0x00, 0x00, 0x02, 0x0F, 0x05
    };
    MachoFileBin::simpleExe(binary, (char*) asmCode, sizeof(asmCode));

    binary.dest();
}