
// local includes
#include "kelvin/sharedmem.h"

// k273 includes
#include <k273/util.h>
#include <k273/logging.h>
#include <k273/exception.h>

// std includes
#include <string>

#include <cerrno>
#include <cstdlib>
#include <memory>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace Kelvin;

///////////////////////////////////////////////////////////////////////////////

SharedMemory::SharedMemory(const string& name, void* pt_mem, size_t size, bool owns) :
    name(name),
    owns(owns),
    mem_size(size),
    pt_mem(pt_mem) {
}

SharedMemory::~SharedMemory() {
    if (this->pt_mem == nullptr || !this->owns) {
        return;
    }

    K273::l_debug("unmapping shared mem");
    ::munmap(this->pt_mem, this->mem_size);

    if (this->owns) {
        K273::l_debug("removing shared mem");
        ::shm_unlink(this->name.c_str());
    }
}

SharedMemory* SharedMemory::create(const string& name, size_t size) {
    // unlink if exists - ignores error
    ::shm_unlink(name.c_str());

    int fd = ::shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);

    if (fd == -1) {
        throw K273::SysException("SharedMemory::create() - failed shm_open()", errno);
    }

    if (::ftruncate(fd, size) == -1) {
        ::close(fd);
        throw K273::SysException("SharedMemory::create() - failed ftruncate()", errno);
    }

    void* pt_mem = ::mmap(0, size, PROT_WRITE, MAP_SHARED, fd, 0);

    if (pt_mem == MAP_FAILED) {
        ::close(fd);
        throw K273::SysException("SharedMemory::create() - failed mmap()", errno);
    }

    K273::l_debug("SharedMemory::create() create shared memory [%s], total size %zu",
                  name.c_str(), size);

    return new SharedMemory(name, pt_mem, size, true);
}

SharedMemory* SharedMemory::attach(const string& name, size_t size) {
    int fd = ::shm_open(name.c_str(), O_RDWR, 0666);

    if (fd == -1) {
        throw K273::SysException("SharedMemory::create() - failed shm_open()", errno);
    }

    void* pt_mem = ::mmap(0, size, PROT_WRITE, MAP_SHARED, fd, 0);

    if (pt_mem == MAP_FAILED) {
        ::close(fd);
        throw K273::SysException("SharedMemory::create() - failed mmap()", errno);
    }

    ::close(fd);

    K273::l_debug("SharedMemory::create() attached to shared memory [%s], total size %zu",
                  name.c_str(), size);

    return new SharedMemory(name, pt_mem, size, false);
}

