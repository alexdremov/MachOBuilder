//
// Created by Александр Дремов on 09.04.2021.
//

#ifndef CLANGUAGE_MACHOHEADERSTR_H
#define CLANGUAGE_MACHOHEADERSTR_H

#include <mach/machine.h>
#include <mach/mach.h>
#include <mach-o/loader.h>
#include <cstring>
#include <cstdio>
#include "../BinFile.h"
#include "loadCommands.h"
#include "../../list/FastList.h"

#define alignSmall 8
#define alignPage 4096

/*
 * Load commands must be aligned to an 8-byte boundary for 64-bit Mach-O files.
 */
struct loadCommand {
    enum loadCommandType {
        LC_TYPE_SEGMENT,
        LC_TYPE_THREAD,
        LC_TYPE_MAIN
    };
    union {
        unixThreadCommand threadSeg;
        segmentCommand64 generalSeg;
        entryPointCommand entrySeg;
    };
    FastList<segmentSection> sections;
    FastList<unsigned> payloads;
    loadCommandType type;
    size_t offset;

    void init() {
        payloads.init();
        sections.init();
    }

    void dest() {
        payloads.dest();
        sections.dest();
    }

    static loadCommand pageZero() {
        loadCommand seg = {};
        seg.init();
        seg.sections.init();
        seg.generalSeg.segment.cmd = LC_SEGMENT_64;
        strcpy(seg.generalSeg.segment.segname, SEG_PAGEZERO);
        seg.generalSeg.segment.vmsize = 0x0000000100000000;
        seg.type = LC_TYPE_SEGMENT;
        return seg;
    }

    static loadCommand code() {
        loadCommand seg = {};
        seg.init();
        seg.generalSeg.segment.cmd = LC_SEGMENT_64;
        strcpy(seg.generalSeg.segment.segname, SEG_TEXT);
        seg.generalSeg.segment.vmsize = 0x0000000000004000;
        seg.generalSeg.segment.maxprot = seg.generalSeg.segment.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
        seg.type = LC_TYPE_SEGMENT;
        return seg;
    }

    static loadCommand main(unsigned segNum) {
        loadCommand seg = {};
        seg.init();
        seg.entrySeg.segment.cmd = LC_MAIN;
        seg.entrySeg.segment.cmdsize = sizeof(seg.entrySeg.segment);
        seg.entrySeg.segment.stacksize = 0;
        seg.entrySeg.sectionIndex = segNum;
        seg.type = LC_TYPE_MAIN;
        return seg;
    }

    static loadCommand thread(unsigned segNum) {
        loadCommand seg = {};
        seg.init();
        seg.threadSeg.segment.thread.cmd = LC_UNIXTHREAD;
        seg.threadSeg.segment.thread.cmdsize = sizeof(seg.threadSeg.segment);
        seg.threadSeg.segment.flavor = x86_THREAD_STATE64;
        seg.threadSeg.segment.count = x86_THREAD_STATE64_COUNT;
        seg.threadSeg.sectionIndex = segNum;
        seg.type = LC_TYPE_THREAD;
        return seg;
    }

    void binWrite(BinFile *out) {
        offset = out->sizeNow;
        switch (type) {
            case LC_TYPE_SEGMENT: {
                BINFILE_WRITE_STRUCT(generalSeg.segment);
                for (size_t it = this->sections.begin(); it != this->sections.end(); this->sections.nextIterator(&it)){
                    segmentSection* section = nullptr;
                    this->sections.get(it, &section);
                    section->offset = out->sizeNow;
                    section->binWrite(out);
                }
                generalSeg.segment.nsects = this->sections.getSize();
                BINFILE_UPDATE(offset, generalSeg.segment, nsects);
                break;
            }
            case LC_TYPE_THREAD: {
                BINFILE_WRITE_STRUCT(threadSeg.segment);
                break;
            }
            case LC_TYPE_MAIN: {
                BINFILE_WRITE_STRUCT(entrySeg.segment);
                break;
            }
        }
        uint32_t cmdsize = out->sizeNow - offset;
        out->writeZeros(cmdsize % alignSmall); // size divisible by 8;
        generalSeg.segment.cmdsize = out->sizeNow - offset;
        BINFILE_UPDATE(offset, generalSeg.segment, cmdsize);
    }
};

struct machHeader64 {

    mach_header_64 header;
    size_t offset;

    static machHeader64 general() {
        machHeader64 header = {};
        header.header.magic = MH_MAGIC_64;
        header.header.cputype = CPU_TYPE_X86_64;
        header.header.cpusubtype = CPU_SUBTYPE_X86_64_ALL | CPU_SUBTYPE_LIB64;
        header.header.filetype = MH_EXECUTE;
        header.header.ncmds = 0; /* to be modified */
        header.header.sizeofcmds = 0; /* to be modified after writing segments */
        header.header.flags = MH_NOUNDEFS;
        return header;
    }

    void binWrite(BinFile *out) {
        offset = out->sizeNow;
        BINFILE_WRITE_STRUCT(header);
    }
};

struct binPayload {
    char *payload;
    size_t size;
    size_t realSize;
    size_t offset;
    bool freeable;
    unsigned align;

    void binWrite(BinFile *out) {
        out->alignZeroes(align);
        offset = out->sizeNow;
        out->write(payload, size);
        out->writeZeros(size % alignSmall);
        realSize = size + size % alignSmall;
    }

    void dest() {
        if (freeable)
            free(payload);
    }
};

#endif //CLANGUAGE_MACHOHEADER_H
