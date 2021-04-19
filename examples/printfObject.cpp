//
// Created by Александр Дремов on 09.04.2021.
//
#include "../src/include/public/MachOBuilder.h"
#include <cstdio>


int main() {
    FILE *res = fopen("machoObject.o", "wb");
    binaryFile binary = {};
    binary.init(res);

    unsigned char asmCode[] = {
            0x55,
            0x48, 0x89, 0xE5,
//            0xCD, 0x03,
            0xE8, 0x00, 0x00, 0x00, 0x00,
            0x31, 0xC0,
            0x5D,
            0xC3
    };
    size_t size = sizeof(asmCode);

    MachoFileBin machoFile = {};
    machoFile.init();
    machoFile.vmAlign = false;
    machoFile.startFromZero = false;

    machoFile.header = machHeader64::object();
    auto codeSegment = loadCommand::codeObject();


    codeSegment.generalSeg.segment.segname[0] = 0;
    auto codeSection = segmentSection::code();
    codeSection.section.align = 4;
    codeSection.relocPayload = 1;
    codeSegment.payloads.pushBack(0);

    binPayload codePayload = {};
    codePayload.payload = (char *) asmCode;
    codePayload.size = size;
    codePayload.freeable = false;
    codePayload.align = 1;
    machoFile.payload.pushBack(codePayload);

    machoFile.sytable.addExternal("__Z8printTenv");
    machoFile.sytable.addInside("_main", 1);

    relocatePayload relPayload = {};
    relPayload.init();
    relPayload.addReloc(0x5, 0, 1, 2, 1, 2);

    codeSection.section.nreloc = relPayload.info.getSize();
    codeSegment.sections.pushBack(codeSection);
    machoFile.loadCommands.pushBack(codeSegment);
    machoFile.payload.pushBack(relPayload.bufferWrite());

    machoFile.loadCommands.pushBack(loadCommand::symtab());
    machoFile.loadCommands.pushBack(loadCommand::dysymtab());


    machoFile.binWrite(&binary);
    machoFile.dest();

    binary.dest();
}