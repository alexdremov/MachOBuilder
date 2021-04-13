//
// Created by Александр Дремов on 11.04.2021.
//

#ifndef MACHOBUILDER_MACHOBUILDER_H
#define MACHOBUILDER_MACHOBUILDER_H

#include <mach-o/loader.h>
#include <mach/machine.h>
#include <mach/mach.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "FastList.h"


#define alignSmall 8
#define alignPage 4096

#define BINFILE_WRITE_STRUCT(structure) out->write((const void*)(&structure), sizeof(structure))
#define BINFILE_WRITE_FIELD(field) out->write((const void*)&field, sizeof(field))
#define BINFILE_WRITE_STRING(str) out->puts((const char*)str);
#define BINFILE_WRITE_FIELD_ALIGNED(field, align) out->write((const void*)&field, sizeof(field), align)
#define BINFILE_WRITE_STRING_ALIGNED(str, align) out->puts((const char*)str, align);
#define BINFILE_UPDATE(offset, structure, field) out->writeOffset(&(structure.field), sizeof((structure.field)),\
                                                 offset + FIELD_OFFSET(decltype(structure), field));

#define FIELD_OFFSET(struct, field) ((size_t)(&(((struct*)nullptr)->field)))


struct binaryFile {
    FILE *file;
    size_t sizeNow;

    void init();

    void init(FILE *setFile, size_t offset = 0);

    void dest();

    size_t writeZeros(size_t number, char useChar = 0);

    size_t alignZeroes(size_t align);

    size_t write(const void *ptr, size_t count, size_t align = 1);

    size_t writeOffset(const void *ptr, size_t count, size_t offset = 0, int pos = SEEK_SET);

    size_t putsOffset(const char *ptr, size_t offset = 0, int pos = SEEK_SET);

    size_t puts(const char *ptr, size_t align = 1);

    int flush();

    static binaryFile *New();

    void Delete();
};

struct segmentCommand64{
    segment_command_64 segment;
};

struct entryPointCommand{
    entry_point_command segment;
    unsigned sectionIndex;
};

struct unixThread{
    thread_command thread;
    uint32_t flavor;
    uint32_t count;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cs;
    uint64_t fs;
    uint64_t gs;
};

struct unixThreadCommand {
    unixThread segment;
    unsigned sectionIndex;
};

struct segmentSection {
    section_64 section;
    size_t offset;

    void binWrite(binaryFile *out);

    static segmentSection code();
};

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

    void init();

    void dest();

    static loadCommand pageZero();

    static loadCommand code();

    static loadCommand main(unsigned segNum);

    static loadCommand thread(unsigned segNum);

    void binWrite(binaryFile *out);
};

struct machHeader64 {

    mach_header_64 header;
    size_t offset;

    static machHeader64 general();

    void binWrite(binaryFile *out);
};

struct binPayload {
    char *payload;
    size_t size;
    size_t realSize;
    size_t offset;
    bool freeable;
    unsigned align;

    void binWrite(binaryFile *out);

    void dest();
};

struct MachoFileBin {
    machHeader64 header;
    FastList<loadCommand> loadCommands;
    FastList<binPayload> payload;

    void init();

    void dest();

    static MachoFileBin *New();

    static void simpleExe(binaryFile& binary, const char* code, size_t size);

    void Delete();

    void binWrite(binaryFile *out);


    void postprocess(binaryFile *out);

    void threadSectionLink(binaryFile *out);

    void mainSectionLink(binaryFile *out);

    void vmRemap(binaryFile *out);

    void payloadsProcess(binaryFile *out);

    void fileOffsetsRemap(binaryFile *out);

    segmentSection *getSectionByIndex(size_t sectionNum, loadCommand** lc= nullptr);
};

#endif //MACHOBUILDER_MACHOBUILDER_H
