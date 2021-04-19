//
// Created by Александр Дремов on 11.04.2021.
//
#include "machoStructure.h"
#include "machoBinFile.h"
#include "public/FastList.h"

void MachoFileBin::init() {
    payload.init();
    sytable.init();
    loadCommands.init();
    header = {};
    vmAlign = true;
    startFromZero = true;
}

void MachoFileBin::dest() {
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommand *tmp = nullptr;
        loadCommands.get(it, &tmp);
        tmp->dest();
    }
    for (size_t it = payload.begin(); it != payload.end(); payload.nextIterator(&it)) {
        binPayload *tmp = nullptr;
        payload.get(it, &tmp);
        tmp->dest();
    }
    sytable.dest();
    loadCommands.dest();
}

MachoFileBin *MachoFileBin::New() {
    auto *thou = static_cast<MachoFileBin *>(calloc(1, sizeof(MachoFileBin)));
    thou->init();
    return thou;
}

void MachoFileBin::Delete() {
    dest();
    free(this);
}

void MachoFileBin::binWrite(binaryFile *out) {
    /*
     * HEADER
     */
    header.header.ncmds = loadCommands.getSize();
    header.binWrite(out);
    /*
     * HEADER FINISHED
     */

    /*
     * LOAD COMMANDS
     */
    size_t lcStart = out->sizeNow;
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommand *tmp = nullptr;
        loadCommands.get(it, &tmp);
        tmp->binWrite(out);
    }
    header.header.sizeofcmds = out->sizeNow - lcStart;
    BINFILE_UPDATE(header.offset, header.header, sizeofcmds);
    /*
     * LOAD COMMANDS FINISHED
     */
    postprocess(out);

    if (out->sizeNow < alignPage)
        out->writeZeros(alignPage - out->sizeNow);
}


void MachoFileBin::postprocess(binaryFile *out) {
    payloadsProcess(out);
    fileOffsetsRemap(out);
    vmRemap(out);
    mainSectionLink(out);
    threadSectionLink(out);
    symbolTableSet(out);
    relocRemap(out);
    dsymUpdate(out);
}

void MachoFileBin::threadSectionLink(binaryFile *out) {
    loadCommand *command = nullptr;
    size_t it = loadCommands.begin();
    for (; it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommands.get(it, &command);
        if (command->type == loadCommand::LC_TYPE_THREAD)
            break;
    }
    if (it == loadCommands.end()) {
        return;
    }
    size_t sectionNum = command->threadSeg.sectionIndex;
    loadCommand *segmentSectionLC = nullptr;
    segmentSection *segmentSection = getSectionByIndex(sectionNum, &segmentSectionLC);
    command->threadSeg.segment.rip = segmentSection->section.addr + segmentSection->section.offset -
                                     segmentSectionLC->generalSeg.segment.fileoff;
    BINFILE_UPDATE(command->offset, command->threadSeg.segment, rip);
}

void MachoFileBin::mainSectionLink(binaryFile *out) {
    loadCommand *command = nullptr;
    size_t it = loadCommands.begin();
    for (; it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommands.get(it, &command);
        if (command->type == loadCommand::LC_TYPE_MAIN)
            break;
    }
    if (it == loadCommands.end()) {
        return;
    }
    size_t sectionNum = command->entrySeg.sectionIndex;
    segmentSection *segmentSection = getSectionByIndex(sectionNum);
    command->entrySeg.segment.entryoff = segmentSection->section.offset;
    BINFILE_UPDATE(command->offset, command->entrySeg.segment, entryoff);
}

void MachoFileBin::vmRemap(binaryFile *out) {
    size_t vmNow = 0;
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        if (vmAlign)
            vmNow += vmNow % alignPage;
        loadCommand *command = nullptr;
        loadCommands.get(it, &command);
        size_t vmStart = vmNow;
        if (command->type == loadCommand::LC_TYPE_SEGMENT) {
            if (command->generalSeg.segment.vmsize < command->generalSeg.segment.filesize)
                command->generalSeg.segment.vmsize = command->generalSeg.segment.filesize;
            if (vmAlign)
                command->generalSeg.segment.vmsize += command->generalSeg.segment.vmsize % alignPage;
            command->generalSeg.segment.vmaddr = vmNow;
            vmNow += command->generalSeg.segment.vmsize;

            BINFILE_UPDATE(command->offset, command->generalSeg.segment, vmsize);
            BINFILE_UPDATE(command->offset, command->generalSeg.segment, vmaddr);

            size_t insideVmNow = vmStart;
            for (size_t itSect = command->sections.begin();
                 itSect != command->sections.end(); command->sections.nextIterator(&itSect)) {
                segmentSection *segmentSection = nullptr;
                command->sections.get(itSect, &segmentSection);
                segmentSection->section.addr = insideVmNow;
                insideVmNow += segmentSection->section.size;
                BINFILE_UPDATE(segmentSection->offset, segmentSection->section, addr);
            }
        }
    }
}

void MachoFileBin::payloadsProcess(binaryFile *out) {
    /*
     * PAYLOAD
     */
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommand *pLoadCommand = nullptr;
        loadCommands.get(it, &pLoadCommand);
        out->alignZeroes(out->sizeNow % alignPage);
        /*
        * SECTIONS DUMP
        */
        size_t segmentsStart = out->sizeNow;
        for (size_t itPayload = pLoadCommand->payloads.begin();
             itPayload != pLoadCommand->payloads.end(); pLoadCommand->payloads.nextIterator(&itPayload)) {
            unsigned *payloadId = nullptr;
            pLoadCommand->payloads.get(itPayload, &payloadId);
            size_t payloadAddr = pLoadCommand->payloads.logicToPhysic(*payloadId);
            binPayload *payloadNow = nullptr;
            payload.get(payloadAddr, &payloadNow);
            payloadNow->binWrite(out);
        }
        for (size_t sectit = pLoadCommand->sections.begin();
             sectit != pLoadCommand->sections.end(); pLoadCommand->sections.nextIterator(&sectit)) {
            segmentSection section = {};
            pLoadCommand->sections.get(sectit, &section);
            if (section.relocPayload != (size_t) -1) {
                binPayload *selectedPayload = nullptr;
                payload.getLogic(section.relocPayload, &selectedPayload);
                selectedPayload->binWrite(out);
            }
        }

        size_t segmentsSize = out->sizeNow - segmentsStart;
        /*
        * SECTIONS DUMP FINISHED
        */
        /*
        * LINKS & UPDATES
        */
        switch (pLoadCommand->type) {
            case loadCommand::LC_TYPE_SEGMENT: {
                pLoadCommand->generalSeg.segment.filesize = segmentsSize;
                pLoadCommand->generalSeg.segment.fileoff = segmentsSize == 0 ? 0 : segmentsStart;
                BINFILE_UPDATE(pLoadCommand->offset, pLoadCommand->generalSeg.segment, filesize);
                BINFILE_UPDATE(pLoadCommand->offset, pLoadCommand->generalSeg.segment, fileoff);

                size_t sectionNum = 0;
                for (size_t itSec = pLoadCommand->sections.begin();
                     itSec != pLoadCommand->sections.end(); pLoadCommand->sections.nextIterator(
                        &itSec), sectionNum++) {
                    segmentSection *segmentSection = nullptr;
                    pLoadCommand->sections.get(itSec, &segmentSection);
                    binPayload *payloadNow = nullptr;
                    payload.get(payload.logicToPhysic(sectionNum), &payloadNow);
                    segmentSection->section.offset = payloadNow->offset;
                    segmentSection->section.size = payloadNow->realSize;
                    BINFILE_UPDATE(segmentSection->offset, segmentSection->section, offset);
                    BINFILE_UPDATE(segmentSection->offset, segmentSection->section, size);
                }
                break;
            }
            case loadCommand::LC_TYPE_THREAD:
            case loadCommand::LC_TYPE_MAIN:
                break;
        }
    }
}

void MachoFileBin::fileOffsetsRemap(binaryFile *out) {
    size_t foffset = 0;
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommand *command = nullptr;
        loadCommands.get(it, &command);
        if (command->type == loadCommand::LC_TYPE_SEGMENT) {
            if (!startFromZero && foffset == 0) {
                foffset = command->generalSeg.segment.fileoff;
            }
            if (command->generalSeg.segment.fileoff > foffset) {
                command->generalSeg.segment.filesize += command->generalSeg.segment.fileoff - foffset;
                command->generalSeg.segment.fileoff = foffset;
            }
            foffset = command->generalSeg.segment.fileoff;

            BINFILE_UPDATE(command->offset, command->generalSeg.segment, fileoff);
            BINFILE_UPDATE(command->offset, command->generalSeg.segment, filesize);
        }
    }
}

segmentSection *MachoFileBin::getSectionByIndex(size_t sectionNum, loadCommand **lc) {
    segmentSection *segmentSectionTmp = nullptr;
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommand *commandTmp = nullptr;
        loadCommands.get(it, &commandTmp);
        if (sectionNum < commandTmp->sections.getSize()) {
            commandTmp->sections.getLogic(sectionNum, &segmentSectionTmp);
            if (lc)
                *lc = commandTmp;
            break;
        }
        sectionNum -= commandTmp->sections.getSize();
    }
    return segmentSectionTmp;
}

void MachoFileBin::simpleExe(binaryFile &binary, const char *code, size_t size) {
    MachoFileBin machoFile = {};
    machoFile.init();

    machoFile.header = machHeader64::executable();
    machoFile.loadCommands.pushBack(loadCommand::pageZero());

    auto codeSection = loadCommand::code();
    codeSection.sections.pushBack(segmentSection::code());
    codeSection.payloads.pushBack(0);
    machoFile.loadCommands.pushBack(codeSection);

    machoFile.loadCommands.pushBack(loadCommand::data());
    machoFile.loadCommands.pushBack(loadCommand::thread(0));

    binPayload codePayload = {};
    codePayload.payload = (char *) code;
    codePayload.size = size;
    codePayload.freeable = false;
    codePayload.align = 1;
    machoFile.payload.pushBack(codePayload);

    machoFile.binWrite(&binary);
    machoFile.dest();
}

void MachoFileBin::relocRemap(binaryFile *out) {
    loadCommand command = {};
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommands.get(it, &command);
        if (command.type != loadCommand::LC_TYPE_SEGMENT)
            continue;
        for (size_t sectit = command.sections.begin();
             sectit != command.sections.end(); command.sections.nextIterator(&sectit)) {
            segmentSection section = {};
            command.sections.get(sectit, &section);
            if (section.relocPayload != (size_t) -1) {
                binPayload selectedPayload = {};
                payload.getLogic(section.relocPayload, &selectedPayload);
                section.section.reloff = selectedPayload.offset;
                BINFILE_UPDATE(section.offset, section.section, reloff);
            }
        }
    }
}

void MachoFileBin::symbolTableSet(binaryFile *out) {
    sytable.binWrite(out);
    sytable.writePayload(out);

    loadCommand command = {};
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommands.get(it, &command);
        if (command.type != loadCommand::LC_TYPE_SYMTAB)
            continue;
        command.symtabSeg.segment.symoff = sytable.offset;
        command.symtabSeg.segment.stroff = sytable.payload.offset;
        command.symtabSeg.segment.strsize = sytable.payload.size;
        command.symtabSeg.segment.nsyms = sytable.storage.getSize();

        BINFILE_UPDATE(command.offset, command.symtabSeg.segment, strsize);
        BINFILE_UPDATE(command.offset, command.symtabSeg.segment, stroff);
        BINFILE_UPDATE(command.offset, command.symtabSeg.segment, symoff);
        BINFILE_UPDATE(command.offset, command.symtabSeg.segment, nsyms);
    }
}

void MachoFileBin::dsymUpdate(binaryFile *out){
    loadCommand* command = nullptr;
    for (size_t it = loadCommands.begin(); it != loadCommands.end(); loadCommands.nextIterator(&it)) {
        loadCommands.get(it, &command);
        if (command->type != loadCommand::LC_TYPE_DYSYMTAB)
            continue;
        unsigned externalSize = 0;
        for (auto &elem: sytable.storage) {
            if (elem.value.external)
                externalSize++;
        }
        command->dysymtabSeg.segment.iextdefsym = 0;
        command->dysymtabSeg.segment.nextdefsym = sytable.storage.getSize() -  externalSize;
        command->dysymtabSeg.segment.iundefsym = sytable.storage.getSize() -  externalSize;
        command->dysymtabSeg.segment.nundefsym = externalSize;

        BINFILE_UPDATE(command->offset, command->dysymtabSeg.segment, nextdefsym);
        BINFILE_UPDATE(command->offset, command->dysymtabSeg.segment, iextdefsym);
        BINFILE_UPDATE(command->offset, command->dysymtabSeg.segment, nundefsym);
        BINFILE_UPDATE(command->offset, command->dysymtabSeg.segment, iundefsym);
    }
}
