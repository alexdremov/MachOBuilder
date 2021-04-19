//
// Created by Александр Дремов on 09.04.2021.
//

#ifndef BinFile_GUARD
#define BinFile_GUARD
#ifndef PUBLIC_HEADER
#include <cstdio>
#include <cstdlib>
#endif

#define BINFILE_WRITE_STRUCT(structure) out->write((const void*)(&structure), sizeof(structure))
#define BINFILE_WRITE_FIELD(field) out->write((const void*)&field, sizeof(field))
#define BINFILE_WRITE_STRING(str) out->puts((const char*)str)
#define BINFILE_WRITE_FIELD_ALIGNED(field, align) out->write((const void*)&field, sizeof(field), align)
#define BINFILE_WRITE_STRING_ALIGNED(str, align) out->puts((const char*)str, align)
#define BINFILE_UPDATE(offset, structure, field) out->writeOffset(&(structure.field), sizeof((structure.field)),\
                                                 offset + FIELD_OFFSET(decltype(structure), field))

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

#endif //BinFile_GUARD
