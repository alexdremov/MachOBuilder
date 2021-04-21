//
// Created by Александр Дремов on 19.04.2021.
//

#include "objectGenerator.h"

void ObjectMachOGen::init() {
    code = nullptr;
    data = nullptr;
    codeSize = 0;
    dataSize = 0;
    relPayload.init();
    machoFile.init();
}

void ObjectMachOGen::dest() {
    relPayload.dest();
    machoFile.dest();
}

void ObjectMachOGen::bindBranchExt(const char *name, size_t offsetName) {
    machoFile.sytable.addExternal(name);
    auto foundIndex = machoFile.sytable.payload.storage.find(name);
    unsigned nameIndex = (*foundIndex).value.index;
    relPayload.addReloc((*foundIndex).key, offsetName, nameIndex,
                        (*foundIndex).value.r_pcrel,
                        (*foundIndex).value.r_length,
                        (*foundIndex).value.r_extern,
                        (*foundIndex).value.r_type);
}

void ObjectMachOGen::bindSignedOffset(const char *name, size_t offsetBind) {
    auto foundIndex = machoFile.sytable.payload.storage.find(name);
    unsigned nameIndex = (*foundIndex).value.index;
    relPayload.addReloc((*foundIndex).key, offsetBind, nameIndex,
                        (*foundIndex).value.r_pcrel,
                        (*foundIndex).value.r_length,
                        (*foundIndex).value.r_extern,
                        (*foundIndex).value.r_type);
}

void ObjectMachOGen::addCode(const char *setCode, size_t size) {
    code = setCode;
    codeSize = size;
}

void ObjectMachOGen::addCode(const unsigned char *setCode, size_t size) {
    code = (char *) setCode;
    codeSize = size;
}

void ObjectMachOGen::addInternalCodeSymbol(const char* symbol, size_t offset){
    machoFile.sytable.addInternal(symbol, 1, offset);
}

void ObjectMachOGen::addInternalDataSymbol(const char* symbol, size_t offset){
    machoFile.sytable.addData(symbol, 2, offset);
}

void ObjectMachOGen::dumpFile(binaryFile &binary) {
    loadCommand codeSegment = {};
    segmentSection codeSection = {};

    generalSetup(codeSegment, codeSection);

    size_t codeSectionPtr = 0;
    codeSegment.sections.pushBack(codeSection, &codeSectionPtr);
    addDataIfNeeded(codeSegment);

    machoFile.loadCommands.pushBack(codeSegment);
    machoFile.loadCommands.pushBack(loadCommand::symtab());
    machoFile.loadCommands.pushBack(loadCommand::dysymtab());

    segmentSection* codeSectionInBinary = nullptr;
    codeSegment.sections.get(codeSectionPtr, &codeSectionInBinary);
    machoFile.payload.pushBack(relPayload.bufferWrite());
    codeSectionInBinary->relocPayload =  machoFile.payload.getSize() - 1;

    machoFile.binWrite(&binary);
}

void ObjectMachOGen::addDataIfNeeded(loadCommand &codeSegment) {
    if (data) {
        auto dataSection = segmentSection::data();
        binPayload dataPayload = {};
        dataPayload.payload = (char *) data;
        dataPayload.size = dataSize;
        dataPayload.freeable = false;
        dataPayload.align = 1;
        machoFile.payload.pushBack(dataPayload);
        codeSegment.sections.pushBack(dataSection);
        codeSegment.payloads.pushBack(machoFile.payload.getSize() - 1);
    }
}

void ObjectMachOGen::generalSetup(loadCommand &codeSegment, segmentSection &codeSection) {
    codeSegment = loadCommand::codeObject();
    codeSection = segmentSection::code();
    machoFile.header = machHeader64::object();
    machoFile.vmAlign = false;
    machoFile.startFromZero = false;

    binPayload codePayload = {};
    codePayload.payload = (char *) code;
    codePayload.size = codeSize;
    codePayload.freeable = false;
    codePayload.align = 1;

    codeSection.section.align = 4;
    codeSection.section.nreloc = relPayload.info.getSize();
    machoFile.payload.pushBack(codePayload);
    codeSegment.payloads.pushBack(machoFile.payload.getSize() - 1);

    for (auto i = machoFile.sytable.storage.begin(); i != machoFile.sytable.storage.end(); i++) {
        if ((*i).value.type == (*i).value.SYM_TYPE_DATA)
            (*i).value.list.n_value += codePayload.size + codePayload.size % alignSmall;
    }

    machoFile.sytable.setSymIdexes();
    for(auto i = relPayload.info.begin(); i != relPayload.info.end(); relPayload.info.nextIterator(&i)) {
        relocationInfo *info = nullptr;
        relPayload.info.get(i, &info);
        auto symbolInTable = machoFile.sytable.storage.find(info->name);
        info->info.r_symbolnum = (*symbolInTable).value.symTabIndex;
    }
}

void ObjectMachOGen::addData(const char *setData, size_t size) {
    dataSize = size;
    data = setData;
}

void ObjectMachOGen::addData(const unsigned char *setData, size_t size) {
    dataSize = size;
    data = (char *) setData;
}
