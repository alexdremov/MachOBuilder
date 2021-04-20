//
// Created by Александр Дремов on 17.04.2021.
//

#include "stringTable.h"

void stringTablePayload::init() {
    offset = 0;
    storage.init();
}

unsigned stringTablePayload::addString(const char *key, uint32_t valOffset,
                                       uint32_t r_pcrel, uint32_t r_length,
                                       uint32_t r_extern, uint32_t r_type) {
    unsigned val = storage.getSize();
    auto res = storage.find(key);
    if (res == storage.end()) {
        storage.set(key, {val, valOffset, r_pcrel, r_length, r_extern, r_type});
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

void symbolTable::addInternal(const char *key, unsigned section, size_t offsetName) {
    unsigned index = payload.addString(key, 0, 1, 2, 1, 2);
    nlist_64 symtbEntry = {};
    symtbEntry.n_type = N_SECT | N_EXT;
    symtbEntry.n_un.n_strx = index;
    symtbEntry.n_sect = section;
    symtbEntry.n_value = offsetName;
    auto found = storage.find(key);

    if (found != storage.end())
        return;
    storage.set(key, {symtbEntry, 0, symbolTableEntry::symbolTableType::SYM_TYPE_INTERNAL});
}

void symbolTable::addData(const char *key, unsigned section, size_t offsetName) {
    unsigned index = payload.addString(key, 0, 1, 2, 1, 1);
    nlist_64 symtbEntry = {};
    symtbEntry.n_type = N_SECT | N_EXT;
    symtbEntry.n_un.n_strx = index;
    symtbEntry.n_sect = section;
    symtbEntry.n_value = offsetName;
    auto found = storage.find(key);

    if (found != storage.end())
        return;
    storage.set(key, {symtbEntry, 0, symbolTableEntry::symbolTableType::SYM_TYPE_DATA});
}

void symbolTable::addExternal(const char *key) {
    unsigned index = payload.addString(key, 0, 1, 2, 1, 2);
    nlist_64 symtbEntry = {};
    symtbEntry.n_type = N_UNDF | N_EXT;
    symtbEntry.n_un.n_strx = index;
    auto found = storage.find(key);
    if (found != storage.end())
        return;
    storage.set(key, {symtbEntry, 0, symbolTableEntry::symbolTableType::SYM_TYPE_EXTERNAL});
}

void symbolTable::setSymIdexes(){
    size_t ind = 0;
    for (auto &elem: storage) {
        if (elem.value.type == symbolTableEntry::SYM_TYPE_DATA) {
            elem.value.symTabIndex = ind++;
        }
    }
    for (auto &elem: storage) {
        if (elem.value.type == symbolTableEntry::SYM_TYPE_INTERNAL) {
            elem.value.symTabIndex = ind++;
        }
    }
    for (auto &elem: storage) {
        if (elem.value.type == symbolTableEntry::SYM_TYPE_EXTERNAL) {
            elem.value.symTabIndex = ind++;
        }
    }
}

void symbolTable::binWrite(binaryFile *out) {
    offset = out->sizeNow;
    size_t ind = 0;
    for (auto &elem: storage) {
        if (elem.value.type == symbolTableEntry::SYM_TYPE_DATA) {
            elem.value.offset = out->sizeNow;
            elem.value.symTabIndex = ind++;
            BINFILE_WRITE_STRUCT(elem.value.list);
        }
    }
    for (auto &elem: storage) {
        if (elem.value.type == symbolTableEntry::SYM_TYPE_INTERNAL) {
            elem.value.offset = out->sizeNow;
            elem.value.symTabIndex = ind++;
            BINFILE_WRITE_STRUCT(elem.value.list);
        }
    }
    for (auto &elem: storage) {
        if (elem.value.type == symbolTableEntry::SYM_TYPE_EXTERNAL) {
            elem.value.offset = out->sizeNow;
            elem.value.symTabIndex = ind++;
            BINFILE_WRITE_STRUCT(elem.value.list);
        }
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
    free(symbols);
}


