//
// Created by Александр Дремов on 11.04.2021.
//

#include "binaryFile.h"


void binaryFile::init() {
    file = nullptr;
    sizeNow = 0;
}

void binaryFile::init(FILE *setFile, size_t offset) {
    file = setFile;
    sizeNow = offset;
}

void binaryFile::dest() {
    fclose(file);
}

size_t binaryFile::writeZeros(size_t number, char useChar) {
    size_t nWritten = 0;
    for (size_t i = 0; i < number; i++) {
        nWritten += fwrite(&useChar, 1, 1, file);
    }
    sizeNow += nWritten;
    return nWritten;
}

size_t binaryFile::alignZeroes(size_t align) {
    return writeZeros(sizeNow % align);
}

size_t binaryFile::write(const void *ptr, size_t count, size_t align) {
    alignZeroes(align);
    size_t nWritten = fwrite(ptr, 1, count, file);
    sizeNow += nWritten;
    return nWritten;
}

size_t binaryFile::writeOffset(const void *ptr, size_t count, size_t offset, int pos) {
    fflush(file);
    fseek(file, offset, pos);
    size_t nWritten = fwrite(ptr, 1, count, file);
    fseek(file, 0, SEEK_END);
    sizeNow = ftell(file);
    return nWritten;
}

size_t binaryFile::putsOffset(const char *ptr, size_t offset, int pos) {
    fseek(file, offset, pos);
    size_t nWritten = fputs(ptr, file);
    fseek(file, 0, SEEK_END);
    sizeNow = ftell(file);
    return nWritten;
}

size_t binaryFile::puts(const char *ptr, size_t align) {
    writeZeros(sizeNow % align);
    size_t nWritten = fputs(ptr, file);
    sizeNow += nWritten;
    return nWritten;
}

int binaryFile::flush() {
    return fflush(file);
}

binaryFile* binaryFile::New() {
    auto *thou = static_cast<binaryFile * > (calloc(1, sizeof(binaryFile)));
    thou->init();
    return thou;
}

void binaryFile::Delete() {
    dest();
    free(this);
}