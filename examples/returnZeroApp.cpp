//
// Created by Александр Дремов on 09.04.2021.
//
#include <MachOBuilder.h>
#include <cstdio>

int main() {
    FILE *res = fopen("machoRetZeroApp", "wb");
    binaryFile binary = {};
    binary.init(res);

    MachoFileBin machoFile = {};
    machoFile.init();

    machoFile.header = machHeader64::general();
    machoFile.loadCommands.pushBack(loadCommand::pageZero());

    auto codeSection = loadCommand::code();
    codeSection.sections.pushBack(segmentSection::code());
    codeSection.payloads.pushBack(0);
    machoFile.loadCommands.pushBack(codeSection);

    machoFile.loadCommands.pushBack(loadCommand::thread(0));

    binPayload codePayload = {};
    unsigned char asmCode[] = {
            0x48, 0x89, 0xC7, 0xB8, 0x01, 0x00, 0x00, 0x02, 0x0F, 0x05
    };
    codePayload.payload = (char *) asmCode;
    codePayload.size = sizeof(asmCode);
    codePayload.freeable = false;
    codePayload.align = 1;
    machoFile.payload.pushBack(codePayload);


    machoFile.binWrite(&binary);

    binary.dest();
    machoFile.dest();
}