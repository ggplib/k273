#pragma once

// k273 includes
#include <k273/util.h>
#include <k273/logging.h>
#include <k273/exception.h>

// std includes
#include <cstring>
#include <atomic>

namespace Kelvin::MsgQ::OneToMany {

    ///////////////////////////////////////////////////////////////////////////////

    // set this per header
    const size_t CACHE_LINE_SIZE = 64;
    const int WORD_SIZE = 8;

    static_assert(sizeof(size_t) == WORD_SIZE, "size_t != WORD_SIZE");
    static_assert(sizeof(std::atomic<size_t>) == WORD_SIZE, "size_t != WORD_SIZE");

    static_assert(((CACHE_LINE_SIZE - 1) & CACHE_LINE_SIZE ) == 0,
                  "CACHE_LINE_SIZE line size must be power of 2" );

    ///////////////////////////////////////////////////////////////////////////////
    // this is a cache-line

    struct CacheLine {
        uint32_t data_count;
        uint32_t skip_count;

        uint8_t data[CACHE_LINE_SIZE - WORD_SIZE];
    };

    ///////////////////////////////////////////////////////////////////////////////

    class Memory {
    public:
        // modulo 2 calculations:
        // basically as long as QUEUE_SIZE is of power of 2, the following works
        // with unsigned arithmetic wrapping (which will take forever to wrap anyway, since size_t
        // is 64 bits)
        //
        CacheLine* getCacheLine(size_t index) {
            return this->buf + index;
        }

    public:
        // padding is to avoid contention by being in first line of cache

        // space to write into (from producer)
        std::atomic <size_t> write_index;
        uint8_t pad__write_index[CACHE_LINE_SIZE - sizeof(std::atomic <size_t>)];

        // what has been read up to (from consumer)
        std::atomic <size_t> consume_index;
        uint8_t pad__consume_index[CACHE_LINE_SIZE - sizeof(std::atomic <size_t>)];

    private:
        CacheLine buf[0];
    };

    ///////////////////////////////////////////////////////////////////////////////
    // note 1-n is, to be more precise, 1-1. The caveat being that a client keeps
    // its own internal state so that it can be a read only consumer.  The
    // reality it doesn't really consume, but piggy back.  The only thing to
    // prevent such a client falling so far behind that it breaks, is having an
    // sufficiently large queue.

    class Producer {

    public:
        Producer(const size_t queue_size) :
            mem(nullptr),
            queue_size(queue_size),
            memory_size(sizeof(Memory) + queue_size * sizeof(CacheLine)),
            acquire_index(0),
            reserved(nullptr) {

            ASSERT_MSG(((this->queue_size - 1) & this->queue_size) == 0, "QueueSize must be power of 2" );
        }

    public:
        void setMemory(void* ptr, bool clear=false) {
            this->mem = reinterpret_cast <Memory*> (ptr);

            if (clear) {
                std::memset(this->mem, 0, this->memory_size);
            }

            this->acquire_index = this->mem->write_index.load(std::memory_order_acquire);
        }

        size_t getNumberCacheLines() const {
            return this->queue_size;
        }

        size_t getMemorySize() const {
            return this->memory_size;
        }

        uint8_t* reserveBytes(size_t len) {

            ASSERT (this->reserved == nullptr);

            // calculate number of cache lines required
            size_t number_of_lines = ((len + sizeof(size_t) - 1) / sizeof(CacheLine)) + 1;

            // normalise to figure out where we are in the queue
            size_t normalized_acquire_index = this->acquire_index % this->queue_size;

            // our goal: is there enough room?
            size_t goal_index = this->acquire_index + number_of_lines;

            // note that this is also used as flag below
            size_t skip_count = 0;

            // we are wanting a contiguous block of memory if we request more than
            // one block.  So if the requested size wraps - we introduce some padding.
            // ok we are going to wrap???
            if (unlikely(normalized_acquire_index + number_of_lines > this->queue_size)) {
                //TRACE("%d + %d > %d", acquire_index, count, this->lines);

                skip_count = this->queue_size - normalized_acquire_index;
                goal_index += skip_count;
            }

            //if (skip_count) {
            //    l_debug("XXX count %d, goal_index %d, skip_count %d, normalized_acquire_index %d, acquire_index %d",
            //           count, goal_index, skip_count, normalized_acquire_index, acquire_index);
            //}

            // ZZZ cache space left...
            size_t consume_index = this->mem->consume_index.load(std::memory_order_acquire);
            if (unlikely(goal_index - consume_index >= this->queue_size)) {
                // ZZZ raise full exception
                ASSERT_MSG(false, "no space left...");
            }

            CacheLine* line = this->mem->getCacheLine(this->acquire_index % this->queue_size);

            // update cache line data
            line->data_count = number_of_lines;
            line->skip_count = skip_count;

            // store for later
            this->reserved = line;

            // ... but we must return the right block (which may included skipped count)
            if (unlikely(skip_count != 0)) {
                ASSERT ((this->acquire_index + skip_count) % this->queue_size == 0);
                line = this->mem->getCacheLine((this->acquire_index + skip_count) % this->queue_size);
            }

            this->acquire_index += number_of_lines + skip_count;

            return line->data;
        }

        void publish() {
            ASSERT (this->reserved != nullptr);
            this->mem->write_index.store(this->acquire_index, std::memory_order_release);
            this->reserved = nullptr;
        }

    private:
        // actual pointer to memory
        Memory* mem;

        const size_t queue_size;
        const size_t memory_size;

        // internal write index, saves looking atomic variable
        size_t acquire_index;

        // pointer to first reserved cached line
        const CacheLine* reserved;
    };

    ///////////////////////////////////////////////////////////////////////////////

    class Consumer {

    public:
        Consumer(const size_t queue_size) :
            mem(nullptr),
            queue_size(queue_size),
            memory_size(sizeof(Memory) + queue_size * sizeof(CacheLine)),
            internal_consume_index(0) {
            ASSERT_MSG(((this->queue_size - 1) & this->queue_size) == 0, "QueueSize must be power of 2" );
        }

        void setMemory(void* ptr, bool clear=false) {
            this->mem = reinterpret_cast <Memory*> (ptr);

            if (clear) {
                std::memset(this->mem, 0, this->memory_size);
            }

            // will set up internal variable to read from the queue
            this->internal_consume_index = this->mem->consume_index;

            // Note there is no protection.  If we fall behind, or the mem->consume_index is
            // overwritten after init() - will do very funky things.
        }

        size_t getNumberCacheLines() const {
            return this->queue_size;
        }

        size_t getMemorySize() const {
            return this->memory_size;
        }

    public:
        // consumer side

        const uint8_t* next(bool consume=false) {
            //l_debug("What is this->internal_consume_index %d", this->internal_consume_index);

            // read is atomic
            size_t last_index = this->mem->write_index.load(std::memory_order_acquire);
            size_t internal = this->internal_consume_index;

            // nothing to read
            if (last_index == internal) {
                //l_debug("here - nothing to read");
                return nullptr;
            }

            const CacheLine* line = this->mem->getCacheLine(internal % this->queue_size);
            this->internal_consume_index += line->data_count + line->skip_count;

            if (line->skip_count) {
                line = this->mem->getCacheLine((internal + line->skip_count) % this->queue_size);
            }

            if (consume) {
                this->mem->consume_index.store(this->internal_consume_index, std::memory_order_release);
            }

            return line->data;
        }

        void consumeAll() {
            this->mem->consume_index.store(this->internal_consume_index, std::memory_order_release);
        }

    private:
        // actual pointer to memory
        Memory* mem;

        const size_t queue_size;
        const size_t memory_size;

        // internal consume index (only used be reader)
        size_t internal_consume_index;
    };

}
