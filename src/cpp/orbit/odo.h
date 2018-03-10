#pragma once

// k273 includes
#include <k273/util.h>

// standard includes
#include <cstdint>
#include <string>
#include <cstring>

namespace K273::Orbit::Odo {

    struct MessageHeader {
        uint32_t message_length;
        uint32_t message_type_id;
        uint8_t data[0];

        template <typename T>
        const T* getPayload() const {
            return reinterpret_cast <const T*> (this->data);
        }

    } PACKED;

    const size_t HEADER_SIZE = sizeof(MessageHeader);

    struct String {
        size_t size() const {
            return this->len;
        }

        const char* c_str() const {
            return reinterpret_cast <const char*> (this->data);
        }

        std::string as_string() const {
            return std::string(this->data, this->len);
        }

    public:
        int16_t len;
        char data[0];

    } PACKED;

    template <typename T> struct OffsetPtr {
    public:
        void set(void* mem) {
            uint8_t* offset_mem = reinterpret_cast <uint8_t*> (mem);
            uint8_t* local = reinterpret_cast <uint8_t*> (this);
            this->offset = offset_mem - local;
        }

        const T& get() const {
            return *(this->internalConstGet());
        }

        const T& operator*() const {
            return *(this->internalConstGet());
        }

        const T* operator->() const {
            return this->internalConstGet();
        }

    private:
        const T* internalConstGet() const {
            const uint8_t* mem = reinterpret_cast <const uint8_t*> (this);
            return reinterpret_cast<const T*> (mem + this->offset);
        }

    private:
        int32_t offset;

    } PACKED;

    ///////////////////////////////////////////////////////////////////////////////
    // iters:

    template <typename T> class PrimitiveIter {
    public:
        PrimitiveIter(int index, const uint8_t* d) :
            index(index),
            next_data(d) {
        }

        bool operator!= (const PrimitiveIter& other) const {
            return this->index != other.index;
        }

        T operator* () const {
            const T* element = reinterpret_cast <const T*> (next_data);
            return *element;
        }

        const PrimitiveIter& operator++ () {
            this->index++;
            this->next_data += sizeof(T);
            return *this;
        }

    private:
        int index;
        const uint8_t* next_data;
    };

    template <typename T> class VariableIter {
    public:
        VariableIter(int index, const uint8_t* d) :
            index(index),
            offset(0),
            next_data(d) {
        }

        bool operator!= (const VariableIter& other) const {
            return this->index != other.index;
        }

        const T* operator* () const {
            const uint8_t* pt_variable = this->next_data + this->offset + sizeof(uint32_t);
            return reinterpret_cast <const T*> (pt_variable);
        }

        const VariableIter& operator++ () {
            this->index++;

            // all we need to do here is skip whatever next_data points to
            const uint32_t* skip_bytes = reinterpret_cast <const uint32_t*> (this->next_data + this->offset);
            this->offset += *skip_bytes;

            return *this;
        }

    private:
        int index;
        int offset;
        const uint8_t* next_data;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // sequences:

    template <typename T> struct PrimitiveSequence {
    public:

        typedef PrimitiveIter<T> Iter;

        size_t size() const {
            return this->len;
        }

        // for fixed size this is O(1) - pointer arithmetic
        T index(size_t i) const {
            const T* elements = reinterpret_cast <const T*> (this->data);
            // this will jump by sizeof(T)
            return *(elements + i);
        }

        Iter begin() const {
            return Iter(0, this->data);
        }

        Iter end() const {
            return Iter(this->size(), nullptr);
        }

    private:
        int32_t len;
        uint8_t data[0];

    } PACKED;

    template <typename T> struct VariableSequence {
    public:

        typedef VariableIter<T> Iter;

        size_t size() const {
            return this->len;
        }

        // this is O(n)
        const T* index(size_t index) const {
            // if more than index... boom
            if (index > this->size()) {
                return nullptr;
            }

            // we jump spin through i times
            const uint8_t* pt_data = this->data;
            for (size_t ii=0; ii<index; ii++) {
                const uint32_t* skip_bytes = reinterpret_cast <const uint32_t*> (pt_data);
                pt_data += *skip_bytes;
            }

            return reinterpret_cast <const T*> (pt_data + sizeof(uint32_t));
        }

        Iter begin() const {
            return Iter(0, this->data);
        }

        Iter end() const {
            return Iter(this->size(), nullptr);
        }

    private:
        uint32_t len;
        uint8_t data[0];

    } PACKED;

    ///////////////////////////////////////////////////////////////////////////////
    // builder helpers:

    class StringBuilder {
    public:
        StringBuilder() {
        }

    public:
        void reset() {
            this->is_set = false;
        }

        bool isSet() const {
            return this->is_set;
        }

        void setMemory(uint8_t* start_memory) {
            this->memory = start_memory;
        }

        uint32_t set(const char* s) {
            // include terminating NULL
            const int len = strlen(s);

            String* sb = reinterpret_cast <String*> (memory);
            sb->len = len;
            memcpy(sb->data, s, len + 1);

            // update internal stuff
            this->is_set = true;

            // round up to nearest 4 (including preceding size)
            const uint32_t bytes_consumed = (((len + sizeof(int16_t)) / 4) + 1) * 4;

            return bytes_consumed;
        }

    private:
        bool is_set;
        uint8_t* memory;
    };

    template <typename T> struct PrimitiveSequenceBuilder {
    public:
        PrimitiveSequenceBuilder() {
        }

    public:
        void reset() {
            this->is_set = false;
            this->count = 0;
        }

        bool isSet() const {
            return this->is_set;
        }

        void setMemory(uint8_t* memory) {
            this->memory_start = reinterpret_cast <uint32_t*> (memory);

            // size here is 4 bytes, best alignment guess
            this->memory_next = reinterpret_cast <T*> (memory + sizeof(uint32_t));
        }

        void pushBack(T value) {
            *this->memory_next = value;
            this->memory_next++;
            this->count++;
        }

        uint32_t finalise() {
            // returns total size
            this->is_set = true;

            *(this->memory_start) = this->count;
            return sizeof(uint32_t) + this->count * sizeof(T);
        }

    private:
        bool is_set;

        int count;
        uint32_t* memory_start;

        T* memory_next;
    };


    template <typename T> struct VariableSequenceBuilder {
    public:
        VariableSequenceBuilder() {
        }

    public:
        void reset() {
            this->is_set = false;
            this->count = 0;

            // even if count zero, we need to say so.
            this->total_size = sizeof(uint32_t);
        }

        bool isSet() const {
            return this->is_set;
        }

        void setMemory(uint8_t* memory) {
            this->memory = reinterpret_cast <uint8_t*> (memory);
        }

        T* getNext() {
            uint8_t* next = this->memory + this->total_size + sizeof(uint32_t);
            this->builder.reset(next);
            return &this->builder;
        }

        void pushBack() {
            uint32_t* entry_size = reinterpret_cast <uint32_t*> (this->memory + this->total_size);

            uint32_t size = this->builder.finalise() + sizeof(uint32_t);
            *entry_size = size;
            this->total_size += size;

            this->count++;
        }

        uint32_t finalise() {
            this->is_set = true;

            uint32_t* set_count = reinterpret_cast <uint32_t*> (this->memory);
            *(set_count) = this->count;

            return this->total_size;
        }

    private:
        bool is_set;

        int count;
        uint32_t total_size;

        uint8_t* memory;

        T builder;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // message:

    template <typename T, uint32_t MSG_TYPE_ID> class MessageBuilder {
    public:
        MessageBuilder() {
        }

    public:
        void reset(uint8_t* memory) {
            this->memory = memory;
            this->header = reinterpret_cast <MessageHeader*> (memory);
        }

        T* getPayloadBuilder() {
            this->builder.reset(this->header->data);
            return &this->builder;
        }

        uint32_t finalise() {
            this->header->message_length = sizeof(MessageHeader) + this->builder.finalise();
            this->header->message_type_id = MSG_TYPE_ID;

            return this->header->message_length;
        }

    private:
        uint8_t* memory;
        MessageHeader* header;
        T builder;
    };

}
