//
// Created by Александр Дремов on 11.04.2021.
//

#ifndef MACHOBUILDER_MACHOBUILDER_TEMPLATE_H
#define MACHOBUILDER_MACHOBUILDER_TEMPLATE_H

#include <mach-o/loader.h>
#include <mach/machine.h>
#include <mach/mach.h>
#include <mach-o/reloc.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "FastList.h"

//
//File contents: HashMasm.h
//
//
// Created by Александр Дремов on 31.03.2021.
//

#ifndef HashMasm_GUARD
#define HashMasm_GUARD
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iterator>
#include "FastList.h"
#include "hash/hashes.h"

template<typename T>
class HashMasm {
    struct HashCell {
        char *key;
        T value;
        size_t hash;
        bool duplicateKey;
        size_t len;


        void dest() {
            if (duplicateKey)
                free((void *) key);
        }
    };

    constexpr static int listInitSize = 8;
    constexpr static int minCapacity = 32;
    FastList<HashCell> *storage;
    bool isRehash;
    size_t capacity;
    unsigned loadRate;
    size_t size;
    size_t threshold;

    size_t hashString(const char *key) {
        #ifdef ASMOPT
            #pragma message "Asm optimization of HashMasm::hashString\n"
            #ifdef USECRC
        return CRC::hash32Asm(reinterpret_cast<const unsigned char *>(key), len);
            #endif
        return FNV::fnv64Asm(key);
        #endif
        #ifndef ASMOPT
        return FNV::fnv64(key);
        #endif
    }

    int rehash() {
        FastList<HashCell> *previousStorage = storage;
        const size_t newCapacity = capacity * 2;
        storage = (FastList<HashCell> *) calloc(newCapacity, sizeof(FastList<HashCell>));
        if (!storage)
            return EXIT_FAILURE;

        initStorage(storage, newCapacity);

        for (size_t i = 0; i < capacity; i++) {
            for (size_t iter = previousStorage[i].begin(); iter != 0; previousStorage[i].nextIterator(&iter)) {
                HashCell *value = nullptr;
                previousStorage[i].get(iter, &value);
                storage[value->hash % newCapacity].pushBack(*value);
            }
        }

        capacity = newCapacity;
        threshold = loadRate * capacity / 100;
        return EXIT_SUCCESS;
    }

    int tryRehash() {
        if (isRehash && size > threshold)
            return rehash();
        return EXIT_SUCCESS;
    }

    void freeStorage(FastList<HashCell> *storageTest) {
        for (size_t i = 0; i < capacity; i++)
            storageTest[i].dest();
        free(storageTest);
    }

    void initStorage(FastList<HashCell> *storageTest, size_t len) {
        for (size_t i = 0; i < len; i++)
            storageTest[i].init(listInitSize);
    }


    HashCell *findCell(const char *key, size_t hashed, size_t *iter = nullptr) {
        HashCell *node = nullptr, *tmpNode = nullptr;
        #ifdef ASMOPT
            #pragma message "Asm optimization of findCell loop\n"
        volatile size_t i = storage[hashed].begin();
        findCellLoop:
        asm goto ( "test %0, %0\n" "je %l1" :/* no output*/: "r"(i): "cc":
        findCellLoopEnd);
        {
            storage[hashed].get(i, &tmpNode);
            if (strcmp(tmpNode->key, key) == 0) {
                node = tmpNode;
                asm goto("jmp %l0":/* no output*/:/* no input */:/* no flags */:findCellLoopEnd);
            }
            storage[hashed].nextIterator((size_t *) &i);
        }
        asm goto("jmp %l0":/* no output*/:/* no input */:/* no flags */:findCellLoop);
        findCellLoopEnd:
        #endif
        #ifndef ASMOPT
        size_t i = storage[hashed].begin();
        for (; i != storage[hashed].end();storage[hashed].nextIterator(&i)){
            storage[hashed].get(i, &tmpNode);
            if (strcmp(tmpNode->key, key) == 0) {
                node = tmpNode;
                break;
            }
        }
        #endif
        if (iter)
            *iter = i;
        return node;
    }

public:
    int init(size_t newCapacity, bool rehash = true, unsigned newLoadrate = 75) {
        if (newCapacity < minCapacity)
            newCapacity = minCapacity;
        capacity = newCapacity * 2;
        loadRate = newLoadrate;
        isRehash = rehash;
        threshold = loadRate * capacity / 100;
        size = 0;
        storage = (FastList<HashCell> *) calloc(capacity, sizeof(FastList<HashCell>));
        if (!storage)
            return EXIT_FAILURE;
        initStorage(storage, capacity);
        return EXIT_SUCCESS;
    }

    int init() {
        return init(minCapacity);
    }

    void dest() {
        freeStorage(storage);
    }

    void set(const char *key, const T &value, bool dublicateKey = true) {
        tryRehash();
        size_t hashedInitial = hashString(key);
        size_t hashed = hashedInitial % capacity;
        char *keyDub = const_cast<char *>(key);
        if (dublicateKey)
            keyDub = strdup(key);

        HashCell *node = findCell(key, hashed);
        if (node) {
            node->value = value;
        } else {
            HashCell newCell = {keyDub, value, hashedInitial, dublicateKey, strlen(keyDub)};
            storage[hashed].pushBack(newCell);
            size++;
        }
    }

    T *get(const char *key) {
        size_t hashed = hashString(key, strlen(key)) % capacity;
        HashCell *node = findCell(key, hashed);
        return node ? &(node->value) : nullptr;
    }

    void remove(const char *key) {
        size_t hashed = hashString(key, strlen(key)) % capacity;
        size_t id = 0;
        findCell(key, hashed, &id);
        if (id) {
            HashCell *found = nullptr;
            storage[hashed].get(id, &found);
            found->dest();
            storage[hashed].remove(id);
            size--;
        }
    }

    T &operator[](const char *key) {
        size_t hashedInitial = hashString(key, strlen(key));
        size_t hashed = hashedInitial % capacity, id = 0;
        findCell(key, hashed, &id);
        if (!id) {
            size++;
            HashCell newCell = {};
            newCell.duplicateKey = true;
            newCell.hash = hashedInitial;
            newCell.key = strdup(key);
            storage[hashed].pushBack(newCell, &id);
        }
        HashCell *value = nullptr;
        storage[hashed].get(id, &value);
        return value->value;
    }

    static HashMasm *New() {
        auto thou = static_cast<HashMasm *>(calloc(1, sizeof(HashMasm)));
        thou->init();
        return thou;
    }

    void Delete() {
        dest();
        free(this);
    }

    [[nodiscard]] bool getIsRehash() const {
        return isRehash;
    }

    [[nodiscard]] size_t getCapacity() const {
        return capacity;
    }

    [[nodiscard]] unsigned int getLoadRate() const {
        return loadRate;
    }

    [[nodiscard]] size_t getSize() const {
        return size;
    }

    [[nodiscard]] size_t getThreshold() const {
        return threshold;
    }

    static int getListInitSize() {
        return listInitSize;
    }

    static int getMinCapacity() {
        return minCapacity;
    }

    void printBucketsSizes(FILE* file=stdout) {
        for (size_t i = 0; i < getCapacity(); i++) {
            fprintf(file, "%zu, ", storage[i].getSize());
        }
        fprintf(file, "\n");
    }

private:
    struct HashIter {
        typedef ptrdiff_t difference_type;
        typedef T value_type;
        typedef value_type &reference;
        typedef value_type *pointer;
        typedef std::bidirectional_iterator_tag iterator_category;

        HashMasm *object;
        size_t bucket;
        size_t pos;
        bool end;

        HashIter &operator++() {
            object->storage[bucket].nextIterator(&pos);
            while (pos == object->storage[bucket].end()) {
                bucket++;
                if (bucket >= object->getCapacity()) {
                    end = true;
                    break;
                }
                pos = object->storage[bucket].begin();
            }
            return *this;
        }

        HashIter &operator--() {
            object->storage[bucket].prevIterator(&pos);
            while (pos == object->storage[bucket].end()) {
                if (bucket == 0) {
                    end = true;
                    break;
                }
                bucket--;
                pos = object->storage[bucket].last();
            }
            return *this;
        }

        HashCell &operator*() {
            HashCell *value = nullptr;
            object->storage[bucket].get(pos, &value);
            return *value;
        }

        HashIter operator++(int) {
            HashIter now = *this;
            ++(*this);
            return now;
        }

        bool operator==(const HashIter &other) const{
            if (end == other.end && end)
                return true;
            return end == other.end && pos == other.pos && bucket == other.bucket && object == other.object;
        }
    };

public:
    HashIter begin() {
        HashIter it = {this, 0, 0, false};
        ++it;
        return it;
    }

    HashIter end() {
        HashIter it = {this, 0, 0, true};
        return it;
    }

    HashIter find(const char *key) {
        size_t hashed = hashString(key) % capacity;
        size_t iter = 0;
        HashCell *node = findCell(key, hashed, &iter);
        if (!node)
            return {this, 0, 0, true};
        return {this, hashed, iter, false};
    }
};

#endif //HashMasm_GUARD
//
//File contents: binaryFile.h
//
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
//
//File contents: loadCommands.h
//
//
// Created by Александр Дремов on 10.04.2021.
//

#ifndef NGGC_LOADCOMMANDS_H
#define NGGC_LOADCOMMANDS_H
#ifndef PUBLIC_HEADER

#include <mach/machine.h>
#include <mach/mach.h>
#include <mach-o/loader.h>
#include "binaryFile.h"

#endif

struct segmentCommand64 {
    segment_command_64 segment;
};

struct entryPointCommand {
    entry_point_command segment;
    unsigned sectionIndex;
};

struct unixThread {
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
    size_t relocPayload;

    void binWrite(binaryFile *out);

    static segmentSection code();
};

struct symtabCommand{
    symtab_command segment;
};

struct dysymtabCommand{
    dysymtab_command segment;
};

#endif //NGGC_LOADCOMMANDS_H
//
//File contents: stringTable.h
//
//
// Created by Александр Дремов on 17.04.2021.
//

#ifndef MACHOBUILDER_STRINGTABLE_H
#define MACHOBUILDER_STRINGTABLE_H

#include <FastList.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include "HashMasm.h"
#include "binaryFile.h"

struct stringIndOffset {
    size_t index;
    size_t offset;
};

struct stringTablePayload {
    HashMasm<stringIndOffset> storage;
    size_t offset;
    size_t size;

    void init();

    unsigned addString(const char *key);

    void dest();

    char** binWrite(binaryFile *out);
};

struct symbolTableEntry{
    nlist_64 list;
    size_t offset;
    bool external;
};

struct symbolTable {
    HashMasm<symbolTableEntry> storage;
    stringTablePayload payload;
    size_t offset;

    void init();

    void dest();

    void addExternal(const char *key);

    void binWrite(binaryFile *out);

    void writePayload(binaryFile *out);

    void addInside(const char *key, unsigned int section, size_t offset = 0);
};

#endif //MACHOBUILDER_STRINGTABLE_H
//
//File contents: machoBinFile.h
//
//
// Created by Александр Дремов on 10.04.2021.
//

#ifndef machoFileBin_GUARD
#define machoFileBin_GUARD
#include "machoStructure.h"
#include "public/FastList.h"
#include "relocateStruct.h"
#include "stringTable.h"

struct MachoFileBin {
    machHeader64 header;
    FastList<loadCommand> loadCommands;
    FastList<binPayload> payload;
    symbolTable sytable;
    bool vmAlign;
    bool startFromZero;

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

    void relocRemap(binaryFile *out);

    void payloadsProcess(binaryFile *out);

    void fileOffsetsRemap(binaryFile *out);

    segmentSection *getSectionByIndex(size_t sectionNum, loadCommand** lc= nullptr);

    void symbolTableSet(binaryFile *pFile);

    void dsymUpdate(binaryFile *out);
};

#endif //machoFileBin_GUARD
//
//File contents: machoStructure.h
//
//
// Created by Александр Дремов on 09.04.2021.
//

#ifndef CLANGUAGE_MACHOHEADERSTR_H
#define CLANGUAGE_MACHOHEADERSTR_H
#ifndef PUBLIC_HEADER

#include <mach/machine.h>
#include <mach/mach.h>
#include <mach-o/reloc.h>
#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <cstring>
#include <cstdio>

#endif

#include "binaryFile.h"
#include "loadCommands.h"
#include "public/FastList.h"

#define alignSmall 8
#define alignPage 4096

/*
 * Load commands must be aligned to an 8-byte boundary for 64-bit Mach-O files.
 */
struct loadCommand {
    enum loadCommandType {
        LC_TYPE_SEGMENT,
        LC_TYPE_THREAD,
        LC_TYPE_MAIN,
        LC_TYPE_DYSYMTAB,
        LC_TYPE_SYMTAB
    };
    union {
        unixThreadCommand threadSeg;
        segmentCommand64 generalSeg;
        entryPointCommand entrySeg;
        symtabCommand symtabSeg;
        dysymtabCommand dysymtabSeg;
    };
    FastList<segmentSection> sections;
    FastList<unsigned> payloads;
    loadCommandType type;
    size_t offset;

    void init();

    void dest();

    static loadCommand pageZero();

    static loadCommand code();

    static loadCommand data();

    static loadCommand symtab();

    static loadCommand dysymtab();

    static loadCommand main(unsigned segNum, size_t stackSize = 0);

    static loadCommand thread(unsigned segNum);

    void binWrite(binaryFile *out);

    static loadCommand codeObject();
};

struct machHeader64 {
    mach_header_64 header;
    size_t offset;

    static machHeader64 executable();

    static machHeader64 object();

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

#endif //CLANGUAGE_MACHOHEADER_H
//
//File contents: relocateStruct.h
//
//
// Created by Александр Дремов on 17.04.2021.
//

#ifndef MACHOBUILDER_RELOCATESTRUCT_H
#define MACHOBUILDER_RELOCATESTRUCT_H
#ifndef PUBLIC_HEADER

#include <mach-o/reloc.h>
#include <binaryFile.h>

#endif

#include "FastList.h"

struct relocatePayload {
    FastList<relocation_info> info;

    void binWrite(binaryFile *out);

    binPayload bufferWrite();

    void addReloc(int32_t r_address, uint32_t r_symbolnum, uint32_t r_pcrel, uint32_t r_length,
                  uint32_t r_extern, uint32_t r_type);

    void addReloc(const relocation_info &other);

    void init();

    void dest();
};


#endif //MACHOBUILDER_RELOCATESTRUCT_H
//
//File contents: objectGenerator.h
//
//
// Created by Александр Дремов on 19.04.2021.
//

#ifndef MACHOBUILDER_OBJECTGENERATOR_H
#define MACHOBUILDER_OBJECTGENERATOR_H
#include "HashMasm.h"
#include "MachOBuilder.h"
#include "binaryFile.h"
#include <cstdio>

struct ObjectMachOGen{
    HashMasm<FastList<size_t>> offsets;
    const char* code;
    size_t codeSize;
    size_t mainOffset;

    void init();

    void dest();

    void addReference(const char* name, size_t offset);

    void addCode(const char* setCode, size_t size);

    void addCode(const unsigned char* setCode, size_t size);

    void setMain(size_t offset);

    void dumpFile(binaryFile& binary);
};

#endif //MACHOBUILDER_OBJECTGENERATOR_H


#endif //MACHOBUILDER_MACHOBUILDER_TEMPLATE_H
