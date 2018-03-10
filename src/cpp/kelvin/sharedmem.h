#pragma once

// k273 includes
#include <k273/util.h>

// std includes
#include <string>

namespace Kelvin {

    class SharedMemory : public K273::NonCopyable {
      public:
        // Will create / recrete if exists.  Takes ownership if creates and if so: destructor
        // responsbile for cleaning up

        SharedMemory(const std::string& name, void* pt_mem, size_t size, bool owns);
        ~SharedMemory();

      public:
        // Tells shared segment name
        std::string getName() const {
            return this->name;
        }

        // Tells size of memory segment
        size_t getSize() const {
            return this->mem_size;
        }

        // Tells size of memory segment
        bool getOwns() const {
            return this->owns;
        }

        // Access to mapped memory
        void* accessMemory() {
            return this->pt_mem;
        }

    public:
        // Creates named shared memory region
        static SharedMemory* create(const std::string& name, size_t size);

        // Attempts to connect to named share memory region
        static SharedMemory* attach(const std::string& name, size_t size);

    private:
        std::string name;
        bool owns;
        size_t mem_size;
        void* pt_mem;
    };

}

