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

    size_t hashString(const char *key, size_t len = 0) {
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
        for (int i = 0; i < capacity; i++)
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
        size_t hashedInitial = hashString(key, strlen(key));
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

        bool operator==(const HashIter &other) {
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
        size_t hashed = hashString(key, strlen(key)) % capacity;
        size_t iter = 0;
        HashCell *node = findCell(key, hashed, &iter);
        if (!node)
            return {this, 0, 0, true};
        return {this, hashed, iter, false};
    }
};

#endif //HashMasm_GUARD
