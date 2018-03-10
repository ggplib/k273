#pragma once

// k273 includes
#include <k273/util.h>
#include <k273/logging.h>
#include <k273/exception.h>

// std includes
#include <cstring>

///////////////////////////////////////////////////////////////////////////////////

namespace K273 {
  namespace QueueManyToOne {

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
        volatile uint32_t data_count;
        uint32_t skip_count;

        uint8_t data[CACHE_LINE_SIZE - WORD_SIZE];
    };

    ///////////////////////////////////////////////////////////////////////////////

    class Memory : NonCopyable {
    public:
        CacheLine* getCacheLine(size_t index) {
            return this->buf + index;
        }

    public:
        // padding is to avoid contention by being in first line of cache

        // space to write into (from producer)
        volatile size_t write_index;
        uint8_t pad__write_index[CACHE_LINE_SIZE - sizeof(std::atomic <size_t>)];

        // what has been read up to (from consumer)
        volatile size_t consume_index;
        uint8_t pad__consume_index[CACHE_LINE_SIZE - sizeof(std::atomic <size_t>)];

        // modulo 2 calculations:
        // basically as long as QUEUE_SIZE is of power of 2, the following works
        // with unsigned arithmetic wrapping.
        //

    private:
        CacheLine buf[0];
    };

    ///////////////////////////////////////////////////////////////////////////////

    class Producer : NonCopyable {
    public:
        Producer(const size_t queue_size) :
            mem(nullptr),
            queue_size(queue_size),
            memory_size(sizeof(Memory) + queue_size * sizeof(CacheLine)),
            reserved(nullptr),
            reserve_count(0) {
            ASSERT_MSG(((this->queue_size - 1) & this->queue_size) == 0, "QueueSize must be power of 2" );
        }

      public:
        void setMemory(void* ptr, bool clear=false) {
            this->mem = reinterpret_cast <Memory*> (ptr);

            if (clear) {
                std::memset(this->mem, 0, this->memory_size);
            }
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

            // cache the write index (may change under our feet)
            size_t acquire_index = this->mem->write_index;

            // normalise to figure out where we are in the queue
            size_t normalized_acquire_index = acquire_index % this->queue_size;

            // our goal: is there enough room?
            size_t goal_index = acquire_index + number_of_lines;

            // note that this is also used as flag below
            size_t skip_count = 0;

            //l_debug("XXX number_of_lines %d, acquire_index %d, normalized_acquire_index %d, goal_index %d, skip_count %d",
            //      number_of_lines, acquire_index, normalized_acquire_index, goal_index, skip_count);

            // we are wanting a contiguous block of memory if we request more than
            // one block.  So if the requested size wraps - we introduce some padding.
            // ok we are going to wrap???

            // ok we are going to wrap???
            if (unlikely(normalized_acquire_index + number_of_lines > this->queue_size)) {
                skip_count = this->queue_size - normalized_acquire_index;
                goal_index += skip_count;
            }

            // read is atomic (plus it will be growing and thus space will be getting larger)
            size_t consume_index = this->mem->consume_index;

            // ZZZ check space left
            if (unlikely(goal_index - consume_index >= this->queue_size)) {
                // ZZZ raise full exception
                ASSERT_MSG(false, "no space left...");
            }

            // so far so good, now reserve this for our own devices
            if (unlikely(__sync_val_compare_and_swap(&this->mem->write_index, acquire_index, goal_index) != acquire_index)) {
                // boo, failed
                l_debug("race condition reserving block %lu %lu", acquire_index, goal_index);
                return nullptr;
            }

            // store for publish
            CacheLine* line = this->reserved = this->mem->getCacheLine(acquire_index % this->queue_size);

            // this is very important - otherwise things fall apart
            ASSERT (line->data_count == 0);

            line->skip_count = skip_count;
            this->reserve_count = number_of_lines;

            // ... but we must return the right block (which may included skipped count)
            if (unlikely(skip_count != 0)) {
                line = this->mem->getCacheLine((acquire_index + skip_count) % this->queue_size);
            }

            return line->data;
        }

        void publish() {
            // publish is to indicate to the consumer we are ready.  From a n
            // producers point of view, we already have reserved our space.
            ASSERT (this->reserved != nullptr);
            __sync_add_and_fetch(&this->reserved->data_count, this->reserve_count);
            this->reserve_count = 0;
            this->reserved = nullptr;
        }

    private:
        // actual pointer to memory
        Memory* mem;

        const size_t queue_size;
        const size_t memory_size;

        // pointer to first reserved block
        CacheLine* reserved;

        // reserve count
        size_t reserve_count;
    };

    ///////////////////////////////////////////////////////////////////////////////

    class Consumer : NonCopyable {

    public:
        Consumer(const size_t queue_size) :
            mem(nullptr),
            queue_size(queue_size),
            memory_size(sizeof(Memory) + queue_size * sizeof(CacheLine)),
            reserved(nullptr) {
            ASSERT_MSG(((this->queue_size - 1) & this->queue_size) == 0, "QueueSize must be power of 2" );
        }

    public:
        void setMemory(void* ptr, bool clear=false) {
            this->mem = reinterpret_cast <Memory*> (ptr);

            if (clear) {
                std::memset(this->mem, 0, this->queue_size);
            }
        }

        size_t getNumberCacheLines() const {
            return this->queue_size;
        }

        size_t getMemorySize() const {
            return this->memory_size;
        }

        uint8_t* next() {
            ASSERT (this->reserved == nullptr);

            // read is atomic
            size_t last_index = this->mem->write_index;

            // shorthand
            size_t consume_index = this->mem->consume_index;

            // full -- ZZZ Exception ???
            if (consume_index == last_index) {
                return nullptr;
            }

            CacheLine* line = this->mem->getCacheLine(consume_index % this->queue_size);

            // read is atomic (see publish() above)
            if (likely(line->data_count)) {
                this->reserved = line;

                if (unlikely(line->skip_count)) {
                    line = this->mem->getCacheLine((consume_index + line->skip_count) % this->queue_size);
                }

                return line->data;
            }

            return nullptr;
        }

        void consume() {
            ASSERT (this->reserved != nullptr);

            // cache
            size_t data_count = this->reserved->data_count;
            size_t skip_count = this->reserved->skip_count;

            //l_debug("Consume data/skip %d/%d", data_count, skip_count);

            // grab reference to reserved
            CacheLine* ptline = this->reserved;

            if (skip_count) {
                // zero out the skipped blocks (why??? ZZZ)
                for (size_t ii=0; ii<skip_count; ii++, ptline++) {
                    // ZZZ yup there is no way these could be non-zero
                    ptline->data_count = 0;
                }

                // to advance we simply go to the start of the buffer
                ptline = this->mem->getCacheLine(0);
            }

            // we need to zero out all cache lines...
            for (size_t ii=0; ii<data_count; ii++, ptline++) {
                ptline->data_count = 0;
            }

            // advance consumed
            __sync_add_and_fetch(&this->mem->consume_index, data_count + skip_count);

            //l_debug("write / consume %d / %d ", this->mem->write_index, this->mem->consume_index);
            this->reserved = nullptr;
        }

    private:
        // actual pointer to memory
        Memory* mem;

        const size_t queue_size;
        const size_t memory_size;

        // pointer to first reserved block
        CacheLine* reserved;
    };
  }
}
