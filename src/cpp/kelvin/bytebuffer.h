#pragma once

// local includes

// k273 includes
#include <k273/exception.h>

// std includes
#include <string>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////

namespace K273::Kelvin {

    class ByteBuffer;

    class BufferUnderflow: public Exception {
    public:
        BufferUnderflow(ByteBuffer* buf);
        virtual ~BufferUnderflow();
    };

    class BufferOverflow: public Exception {
    public:
        BufferOverflow(ByteBuffer* buf);
        virtual ~BufferOverflow();
    };

    ///////////////////////////////////////////////////////////////////////////

    class ByteBuffer {
    private:
        static constexpr int BLOCKSIZE = 1024;

    public:
        ByteBuffer(int capacity=BLOCKSIZE, char* buffer=nullptr);
        ~ByteBuffer();

    public:
        // api

        // clears out buffer completely by pos/limit, not actually changing the contents
        void clear() {
            this->pos = 0;
            this->limit = this->capacity;
        }

        int remaining() const {
            return this->limit - this->pos;
        }

        // call flip to start reading back what was written
        void flip() {
            this->limit = this->pos;
            this->pos = 0;
        }

        // similar to clear, but move any extranous data back to pos 0
        void compact();

        void mark() {
            this->markpos = this->pos;
        }

        void resetFromMark() {
            this->pos = this->markpos;
        }

        void skip(int size) {
            if (unlikely(size > this->remaining())) {
                throw BufferUnderflow(this);
            }

            this->pos += size;
        }

        // Be careful...
        char* getInternalBuf(int incr=0) {
            return this->buf + this->pos + incr;
        }

        // gets
        template <typename T>
        T getDatatype() {
            int size = sizeof(T);
            if (unlikely(size > this->remaining())) {
                throw BufferUnderflow(this);
            }

            T* pt_data = reinterpret_cast<T*> (this->buf + this->pos);
            this->pos += size;
            return *pt_data;
        }

        bool getBool() {
            return this->getDatatype<uint8_t>() != 0;
        }


        std::string getString(int size) {
            if (unlikely(size > this->remaining())) {
                throw BufferUnderflow(this);
            }

            std::string data(this->buf + this->pos, size);
            this->pos += size;

            return data;
        }

        // puts
        template <typename T>
        void putDatatype(T data) {
            int size = sizeof(T);
            if (unlikely(size > this->remaining())) {
                throw BufferOverflow(this);
            }

            T* pt_data = reinterpret_cast<T*> (this->buf + this->pos);
            *pt_data = data;
            this->pos += size;
        }

        void putBool(bool data) {
            this->putDatatype<uint8_t> (data ? 1 : 0);
        }

        void putString(const std::string& data, int size=-1) {
            // default size, entire c++-11 strings are null terminated internally (even then
            // c_str() will always be null terminated).

            if (size == -1) {
                size = data.size() + 1;
            }

            if (unlikely(size > this->remaining())) {
                throw BufferOverflow(this);
            }

            std::memcpy(this->buf + this->pos, data.c_str(), size);
            this->pos += size;
        }

        // read and write arbitrary data
        void read(char* data, int size) {
            if (unlikely(size > this->remaining())) {
                throw BufferUnderflow(this);
            }

            std::memcpy(data, this->buf + this->pos, size);
            this->pos += size;
        }

        void write(const char* data, int size) {
            if (unlikely(size > this->remaining())) {
                throw BufferOverflow(this);
            }

            std::memcpy(this->buf + this->pos, data, size);
            this->pos += size;
        }

        std::string repr() const;

    private:
        int pos;
        int markpos;

        int limit;
        int capacity;

        char* buf;
        const bool own_memory;
    };
}
