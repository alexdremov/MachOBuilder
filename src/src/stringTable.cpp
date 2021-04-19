//
// Created by Александр Дремов on 17.04.2021.
//

#include "stringTable.h"

void stringTablePayload::init() {
    offset = 0;
    storage.init();
}

unsigned stringTablePayload::addString(const char *key) {
    unsigned val = storage.getSize();
    auto res = storage.find(key);
    if (res == storage.end()) {
        storage.set(key, {val, 0});
        return val;
    }
    return (*res).value.index;
}

void stringTablePayload::dest() {
    for (auto &elem: storage)
        elem.dest();
    storage.dest();
}

char **stringTablePayload::binWrite(binaryFile *out) {
    offset = out->sizeNow;
    char **sorted = (char **) calloc(storage.getSize(), sizeof(char *));
    for (const auto &elem: storage) {
        sorted[elem.value.index] = elem.key;
    }

    size = 0;
    for (unsigned i = 0; i < storage.getSize(); i++) {
        out->writeZeros(1);
        auto found = storage.find(sorted[i]);
        (*found).value.offset = out->sizeNow;
        out->puts(sorted[i]);
        out->writeZeros(1);
        size += 2 + strlen(sorted[i]);
    }
    return sorted;
}

void symbolTable::init() {
    storage.init();
    payload.init();
}

void symbolTable::dest() {
    storage.dest();
    payload.dest();
}

void symbolTable::addInside(const char *key, unsigned section, size_t offset) {
    unsigned index = payload.addString(key);
    nlist_64 symtbEntry = {};
    symtbEntry.n_type = N_SECT | N_EXT;
    symtbEntry.n_un.n_strx = index;
    symtbEntry.n_sect = section;
    symtbEntry.n_value = offset;
    auto found = storage.find(key);
    if (found != storage.end())
        return;
    storage.set(key, {symtbEntry, 0, false});
}

void symbolTable::addExternal(const char *key) {
    unsigned index = payload.addString(key);
    nlist_64 symtbEntry = {};
    symtbEntry.n_type = N_UNDF | N_EXT;
    symtbEntry.n_un.n_strx = index;
    auto found = storage.find(key);
    if (found != storage.end())
        return;
    storage.set(key, {symtbEntry, 0, true});
}

void symbolTable::binWrite(binaryFile *out) {
    offset = out->sizeNow;
    for (auto &elem: storage) {
        if (!elem.value.external)
            continue;
        elem.value.offset = out->sizeNow;
        BINFILE_WRITE_STRUCT(elem.value.list);
    }
    for (auto &elem: storage) {
        if (elem.value.external)
            continue;
        elem.value.offset = out->sizeNow;
        BINFILE_WRITE_STRUCT(elem.value.list);
    }
}

void symbolTable::writePayload(binaryFile *out) {
    char **symbols = payload.binWrite(out);
    for (auto &elem: storage) {
        unsigned index = elem.value.list.n_un.n_strx;
        auto foundPayload = payload.storage.find(symbols[index]);
        elem.value.list.n_un.n_strx = (*foundPayload).value.offset - payload.offset;
        BINFILE_UPDATE(elem.value.offset, elem.value.list, n_un.n_strx);
    }
}


