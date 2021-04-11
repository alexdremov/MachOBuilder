//
// Created by Александр Дремов on 31.03.2021.
//

#ifndef FastList_GUARD
#define FastList_GUARD

template<typename T>
class FastList {
    enum ListOpResult {
        LIST_OP_OK,
        LIST_OP_NOMEM,
        LIST_OP_CORRUPTED,
        LIST_OP_OVERFLOW,
        LIST_OP_UNDERFLOW,
        LIST_OP_NOTFOUND,
        LIST_OP_SEGFAULT
    };

    struct ListNode {
        size_t next;
        size_t previous;
        T value;
        bool valid;
    };

    ListNode *storage;
    bool optimized;
    constexpr static int INITIAL_INCREASE = 32;

    size_t capacity;
    size_t size;

    size_t freePtr;
    size_t freeSize;
public:

    void init() {
        init(INITIAL_INCREASE);
    }

    void init(size_t initialSize) {
        optimized = true;
        capacity = initialSize;
        size = 0;
        freeSize = 0;
        freePtr = 0;
        this->storage = (ListNode *) calloc(initialSize + 1, sizeof(ListNode));
        this->storage[0].next = 0;
        this->storage[0].previous = 0;
        this->storage[0].valid = false;
    }

    void dest() {
        free(this->storage);
    }

    static FastList *New() {
        auto thou = static_cast<FastList *>(calloc(1, sizeof(FastList)));
        thou->init();
        return thou;
    }

    void Delete() {
        dest();
        free(this);
    }

    /**
     * Retrieves next possible free pos at all costs.
     * Reallocates container if needed.
     */
    size_t getFreePos(bool mutating = false) {
        if (freeSize != 0) {
            size_t newPos = this->freePtr;
            if (mutating) {
                this->freeSize--;
                this->freePtr = this->storage[newPos].next;
            }
            this->storage[newPos].valid = true;
            return newPos;
        }
        if (this->reallocate() != LIST_OP_OK)
            return 0;
        this->storage[this->size + 1].valid = true;
        return this->size + 1;
    }

    /**
     * Reallocates container so that it can hold one more value
     * Reallocation is not performed if some freeSize cells are available.
     */
    ListOpResult reallocate() {
        if (this->freeSize != 0)
            return LIST_OP_OK;
        if (this->size < this->capacity)
            return LIST_OP_OK;
        size_t newCapacity = this->capacity;
        if (this->size >= this->capacity)
            newCapacity = (this->capacity == 0) ? INITIAL_INCREASE : this->capacity * 2;

        if (this->capacity == newCapacity + 2)
            return LIST_OP_OK;

        auto *newStorage = (ListNode *) realloc(this->storage, (newCapacity + 2) * sizeof(ListNode));

        if (newStorage == NULL)
            return LIST_OP_NOMEM;
        this->storage = newStorage;
        this->capacity = newCapacity;
        return LIST_OP_OK;
    }

    /**
     * Add released cell to the free poses list
     */
    void addFreePos(size_t pos) {
        this->storage[pos].valid = false;
        this->storage[pos].previous = pos;
        this->storage[pos].next = pos;
        if (this->freeSize == 0) {
            this->freeSize = 1;
            this->freePtr = pos;
        } else {
            this->freeSize++;
            this->storage[pos].next = this->freePtr;
            this->freePtr = pos;
        }
    }

    /**
     * Convert logic position to the physic one
     */
    size_t logicToPhysic(size_t pos) const {
        if (this->optimized) {
            return pos + 1;
        } else {
            size_t iterator = 0;
            for (size_t i = 0; i <= pos; i++)
                iterator = this->storage[iterator].next;
            return iterator;
        }
    }

    /**
     * Insert an element after pos
     * @param pos - physical pos of considered element
     * @param value - value to be inserted
     * @param physPos - physical position of inserted element
     * @return operation result
     */
    ListOpResult insertAfter(size_t pos, T value, size_t *physPos = nullptr) {
        if (pos > this->sumSize()) {
            return LIST_OP_OVERFLOW;
        }
        if (!this->addressValid(pos) && pos != 0) {
            return LIST_OP_SEGFAULT;
        }
        if (pos != this->storage[0].previous)
            this->optimized = false;

        size_t newPos = this->getFreePos(true);
        if (newPos == 0)
            return LIST_OP_NOMEM;

        if (physPos != nullptr)
            *physPos = newPos;

        this->storage[newPos].value = value;
        this->storage[newPos].previous = pos;
        this->storage[newPos].next = this->storage[pos].next;

        this->storage[this->storage[pos].next].previous = newPos;
        this->storage[pos].next = newPos;

        this->size++;

        return LIST_OP_OK;
    }

    /**
     * Insert an element after pos
     * @param pos - logical pos of considered element
     * @param value - value to be inserted
     * @param physPos - physical position of inserted element
     * @return operation result
     */
    ListOpResult insertAfterLogic(size_t pos, T value, size_t *physPos = nullptr) {
        if (pos > this->size)
            return LIST_OP_OVERFLOW;
        return this->insertAfter(this->logicToPhysic(pos), value, physPos);
    }

    /**
     * Insert an element before pos
     * @param pos - physical pos of considered element
     * @param value - value to be inserted
     * @param physPos - physical position of inserted element
     * @return operation result
     */
    ListOpResult insertBefore(size_t pos, T value, size_t *physPos = nullptr) {
        if (pos > this->sumSize() || (!this->addressValid(pos) && pos != 0))
            return LIST_OP_SEGFAULT;
        pos = this->storage[pos].previous;
        return this->insertAfter(pos, value, physPos);
    }

    /**
     * Insert an element before pos
     * @param pos - logical pos of considered element
     * @param value - value to be inserted
     * @param physPos - physical position of inserted element
     * @return operation result
     */
    ListOpResult insertBeforeLogic(size_t pos, T value, size_t *physPos = nullptr) {
        if (pos > this->size)
            return LIST_OP_OVERFLOW;
        return this->insertBefore(this->logicToPhysic(pos), value, physPos);
    }

    /**
     * Insert an element at the first position
     * @param value - value to be inserted
     * @param physPos - physical position of inserted element
     * @return operation result
     */
    ListOpResult pushFront(const T &value, size_t *physPos = nullptr) {
        return this->insertAfter(0, value, physPos);
    }

    /**
     * Insert an element at the last position
     * @param value - value to be inserted
     * @param physPos - physical position of inserted element
     * @return operation result
     */
    ListOpResult pushBack(const T &value, size_t *physPos = nullptr) {
        return this->insertAfter(this->storage[0].previous, value, physPos);
    }

    /**
     * Set an element at the physical position pos to the new value
     * @param pos - physical pos of considered element
     * @param value - new value
     * @return operation result
     */
    ListOpResult set(size_t pos, const T &value) {
        if (!this->addressValid(pos))
            return LIST_OP_SEGFAULT;
        this->storage[pos].value = value;
        return LIST_OP_OK;
    }

    /**
     * Set an element at the logical position pos to the new value
     * @param pos - logical pos of considered element
     * @param value - new value
     * @return operation result
     */
    ListOpResult setLogic(size_t pos, const T &value) {
        if (pos > this->size)
            return LIST_OP_OVERFLOW;
        return this->set(this->logicToPhysic(pos), value);
    }

    /**
     * Get an element at the physical position pos
     * @param pos - physical pos of considered element
     * @param value - retrieved value
     * @return operation result
     */
    ListOpResult get(size_t pos, T **value) {
        if (value == nullptr || !this->storage[pos].valid)
            return LIST_OP_SEGFAULT;
        *value = &(this->storage[pos].value);
        return LIST_OP_OK;
    }

    const ListNode *getStorage() const {
        return storage;
    }

    /**
     * Get an element at the logical position pos
     * @param pos - logical pos of considered element
     * @param value - retrieved value
     * @return operation result
     */
    ListOpResult getLogic(size_t pos, T **value = nullptr) {
        if (pos > this->size)
            return LIST_OP_OVERFLOW;
        return this->get(this->logicToPhysic(pos), value);
    }

    /**
     * Retrieve an element at the physical position pos and remove it
     * @param pos - physical pos of considered element
     * @param value - retrieved value
     * @return operation result
     */
    ListOpResult pop(size_t pos, T *value = nullptr) {
        if (this->size == 0)
            return LIST_OP_UNDERFLOW;
        if (!this->addressValid(pos))
            return LIST_OP_SEGFAULT;

        if (pos != this->storage[0].previous)
            this->optimized = false;

        if (value != nullptr)
            *value = this->storage[pos].value;

        this->storage[this->storage[pos].next].previous = this->storage[pos].previous;
        this->storage[this->storage[pos].previous].next = this->storage[pos].next;

        this->addFreePos(pos);
        this->size--;
        return LIST_OP_OK;
    }

    /**
     * Retrieve an element at the beginning and remove it
     * @param value - retrieved value
     * @return operation result
     */
    ListOpResult popFront(T *value) {
        return this->pop(this->storage[0].next, value);
    }

    /**
     * Retrieve an element at the end and remove it
     * @param value - retrieved value
     * @return operation result
     */
    ListOpResult popBack(T *value) {
        return this->pop(this->storage[0].previous, value);
    }

    /**
     * Retrieve an element at the logical position pos and remove it
     * @param pos - logical pos of considered element
     * @param value - retrieved value
     * @return operation result
     */
    ListOpResult popLogic(size_t pos, T *value) {
        return this->pop(this->logicToPhysic(pos), value);
    }

    /**
     * Remove an element at the physical position pos
     * @param pos - physical pos of considered element
     * @return operation result
     */
    ListOpResult remove(size_t pos) {
        return this->pop(pos, nullptr);
    }

    /**
     * Remove an element at the logical position pos
     * @param pos - logical pos of considered element
     * @return operation result
     */
    ListOpResult removeLogic(size_t pos) {
        if (pos > this->size)
            return LIST_OP_OVERFLOW;
        return this->pop(this->logicToPhysic(pos), nullptr);
    }

    /**
     * Clears the list
     * @return operation result
     */
    ListOpResult clear() {
        this->size = 0;
        this->storage[0].next = 0;
        this->storage[0].previous = 0;
        this->freeSize = 0;
        this->freePtr = 0;
        this->reallocate();
        return LIST_OP_OK;
    }

    /**
     * Optimizes the list so that logical access is effective and
     * physical positions are aligned in ascending order in the storage
     * @return operation result
     */
    ListOpResult optimize() {
        auto *newStorage = (ListNode *) (calloc(this->size + 2, sizeof(ListNode)));
        if (newStorage == nullptr)
            return LIST_OP_NOMEM;
        newStorage[0] = this->storage[0];
        size_t iterator = this->storage[0].next;
        for (size_t i = 0; i < this->size; i++) {
            newStorage[i + 1] = this->storage[iterator];
            iterator = this->storage[iterator].next;
            newStorage[i + 1].previous = i;
            newStorage[i].next = i + 1;
            newStorage[i].valid = true;
            if (i + 1 == this->size) {
                newStorage[i + 1].next = 0;
            }
        }
        this->optimized = true;
        this->freePtr = 0;
        this->freeSize = 0;
        free(this->storage);
        this->storage = newStorage;
        this->capacity = this->size;
        return LIST_OP_OK;
    }

    /**
     * Moves iterator to the next physical position
     * @param pos
     * @return
     */
    ListOpResult nextIterator(size_t *pos) {
        if (!this->addressValid(*pos) && *pos != 0)
            return LIST_OP_SEGFAULT;
        *pos = this->storage[*pos].next;
        return LIST_OP_OK;
    }

    /**
     * Moves iterator to the next physical position
     * @param pos
     * @return
     */
    size_t nextIterator(size_t pos) {
        if (!this->addressValid(pos) && pos != 0)
            return 0;
        return this->storage[pos].next;
    }

    /**
     * Moves iterator to the next physical position
     * @param pos
     * @return
     */
    size_t prevIterator(size_t pos) {
        if (!this->addressValid(pos) && pos != 0)
            return 0;
        return this->storage[pos].previous;
    }

    /**
     * Moves iterator to the next physical position
     * @param pos
     * @return
     */
    ListOpResult prevIterator(size_t *pos) {
        if (!this->addressValid(*pos) && *pos != 0)
            return LIST_OP_SEGFAULT;
        *pos = this->storage[*pos].previous;
        return LIST_OP_OK;
    }

    /**
     * Resizes list to desired number of elements. If elements number is lower than current one,
     * shrinks to fit.
     * @return operation result
     */
    ListOpResult resize(size_t elemNumbers) {
        if (elemNumbers < this->sumSize())
            elemNumbers = this->sumSize();
        elemNumbers++;
        auto newStorage = (ListNode *) calloc(elemNumbers, sizeof(ListNode));
        if (newStorage == nullptr)
            return LIST_OP_NOMEM;
        this->storage = newStorage;
        this->capacity = elemNumbers - 1;
        return LIST_OP_OK;
    }

    /**
     * Resizes list to the minimum available space
     * @return operation result
     */
    ListOpResult shrinkToFit() {
        return this->resize(0);
    }

    [[nodiscard]] size_t begin() const {
        #ifdef ASMOPT
            #pragma message "Asm optimization of FastList::begin\n"
        volatile size_t retVal = 0;
        asm volatile(
            "mov (%1), %0\n"
            "mov (%0), %0\n"
            : "=r"(retVal)
            :"r"(&(this->storage)));
        return retVal;
        #endif
        #ifndef ASMOPT
        return this->storage[0].next;
        #endif
    }

    [[nodiscard]] size_t last() const {
        return this->storage[0].previous;
    }

    [[nodiscard]] size_t end() const {
        return 0;
    }

    [[nodiscard]] size_t getSize() const {
        return this->size;
    }

    [[nodiscard]] size_t getCapacity() const {
        return this->capacity;
    }

    [[nodiscard]] bool isOptimized() const {
        return this->optimized;
    }

    [[nodiscard]]bool isEmpty() const {
        return this->size == 0;
    }

    [[nodiscard]] size_t sumSize() const {
        return this->size + this->freeSize;
    }

    [[nodiscard]] bool addressValid(size_t pos) const {
        return this->storage[pos].valid;
    }
};

#endif //FastList_GUARD
