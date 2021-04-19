//
// Created by Александр Дремов on 19.04.2021.
//

#include "objectGenerator.h"

void ObjectMachOGen::init(){
    mainOffset = 0;
    code = nullptr;
    offsets.init();
}

void ObjectMachOGen::dest(){
    offsets.dest();
}

void ObjectMachOGen::addReference(const char* name, size_t offset){
    auto found = offsets.find(name);
    if (found == offsets.end()){
        FastList<size_t> list = {};
        list.init();
        offsets.set(name, list);
        found = offsets.find(name);
    }
    (*found).value.pushBack(offset);
}

void ObjectMachOGen::addCode(const char* setCode, size_t size){
    code = setCode;
    codeSize = size;
}

void ObjectMachOGen::addCode(const unsigned char* setCode, size_t size){
    code = (char*)setCode;
    codeSize = size;
}

void ObjectMachOGen::setMain(size_t offset){
    mainOffset = offset;
}

void ObjectMachOGen::dumpFile(binaryFile& binary){
    MachoFileBin machoFile = {};
    machoFile.init();
    machoFile.vmAlign = false;
    machoFile.startFromZero = false;

    machoFile.header = machHeader64::object();
    auto codeSegment = loadCommand::codeObject();
    auto codeSection = segmentSection::code();
    codeSection.section.align = 4;
    codeSection.relocPayload = 1;
    codeSegment.payloads.pushBack(0);

    binPayload codePayload = {};
    codePayload.payload = (char *) code;
    codePayload.size = codeSize;
    codePayload.freeable = false;
    codePayload.align = 1;
    machoFile.payload.pushBack(codePayload);

    for (auto elem: offsets)
        machoFile.sytable.addExternal(elem.key);
    machoFile.sytable.addInside("_main", 1, mainOffset);

    relocatePayload relPayload = {};
    relPayload.init();

    for (auto elem: offsets){
        auto foundIndex = machoFile.sytable.payload.storage.find(elem.key);
        unsigned nameIndex = (*foundIndex).value.index;
        for(auto i = elem.value.begin(); i != elem.value.end(); elem.value.nextIterator(&i)){
            size_t fileOffset = 0;
            elem.value.get(i, &fileOffset);
            relPayload.addReloc(fileOffset, nameIndex, 1, 2, 1, 2);
        }
    }

    codeSection.section.nreloc = relPayload.info.getSize();
    codeSegment.sections.pushBack(codeSection);
    machoFile.loadCommands.pushBack(codeSegment);
    machoFile.payload.pushBack(relPayload.bufferWrite());

    machoFile.loadCommands.pushBack(loadCommand::symtab());
    machoFile.loadCommands.pushBack(loadCommand::dysymtab());

    machoFile.binWrite(&binary);
    machoFile.dest();
}