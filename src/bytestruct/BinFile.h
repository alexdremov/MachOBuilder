//
// Created by Александр Дремов on 09.04.2021.
//

#ifndef BinFile_GUARD
#define BinFile_GUARD
#include <cstdio>
#include <cstdlib>

#define BINFILE_WRITE_STRUCT(structure) out->write((const void*)(&structure), sizeof(structure))
#define BINFILE_WRITE_FIELD(field) out->write((const void*)&field, sizeof(field))
#define BINFILE_WRITE_STRING(str) out->puts((const char*)str);
#define BINFILE_WRITE_FIELD_ALIGNED(field, align) out->write((const void*)&field, sizeof(field), align)
#define BINFILE_WRITE_STRING_ALIGNED(str, align) out->puts((const char*)str, align);
#define BINFILE_UPDATE(offset, structure, field) out->writeOffset(&(structure.field), sizeof((structure.field)),\
                                                 offset + FIELD_OFFSET(decltype(structure), field));

#define FIELD_OFFSET(struct, field) ((size_t)(&(((struct*)nullptr)->field)))

struct BinFile {
    FILE *file;
    size_t sizeNow;

    void init() {
        file = nullptr;
        sizeNow = 0;
    }

    void init(FILE *setFile, size_t offset = 0) {
        file = setFile;
        sizeNow = offset;
    }

    void dest() {
        fclose(file);
    }

    size_t writeZeros(size_t number, char useChar = 0) {
        size_t nWritten = 0;
        for (size_t i = 0; i < number; i++) {
            nWritten += fwrite(&useChar, 1, 1, file);
        }
        sizeNow += nWritten;
        return nWritten;
    }

    size_t alignZeroes(size_t align) {
        return writeZeros(sizeNow % align);
    }

    size_t write(const void *ptr, size_t count, size_t align = 1) {
        alignZeroes(align);
        size_t nWritten = fwrite(ptr, 1, count, file);
        sizeNow += nWritten;
        return nWritten;
    }

    size_t writeOffset(const void *ptr, size_t count, size_t offset = 0, int pos = SEEK_SET) {
        fseek(file, offset, pos);
        size_t nWritten = fwrite(ptr, 1, count, file);
        fseek(file, 0, SEEK_END);
        sizeNow = ftell(file);
        return nWritten;
    }

    size_t putsOffset(const char *ptr, size_t offset = 0, int pos = SEEK_SET) {
        fseek(file, offset, pos);
        size_t nWritten = fputs(ptr, file);
        fseek(file, 0, SEEK_END);
        sizeNow = ftell(file);
        return nWritten;
    }

    size_t puts(const char *ptr, size_t align = 1) {
        writeZeros(sizeNow % align);
        size_t nWritten = fputs(ptr, file);
        sizeNow += nWritten;
        return nWritten;
    }

    int flush() {
        return fflush(file);
    }

    static BinFile *New() {
        BinFile *thou = static_cast<BinFile * > (calloc(1, sizeof(BinFile)));
        thou->init();
        return thou;
    }

    void Delete() {
        dest();
        free(this);
    }

};

#endif //BinFile_GUARD
