
// local includes
#include "kelvin/bytebuffer.h"

// k273 includes
#include <k273/logging.h>
#include <k273/exception.h>
#include <k273/strutils.h>

// std includes
#include <cstring>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273::Kelvin;

///////////////////////////////////////////////////////////////////////////////

BufferUnderflow::BufferUnderflow(ByteBuffer* buf) :
    Exception(K273::fmtString("BufferUnderflow : %s", buf->repr().c_str())) {
}

BufferUnderflow::~BufferUnderflow() {
}

BufferOverflow::BufferOverflow(ByteBuffer* buf) :
    Exception(K273::fmtString("BufferOverflow : %s @ %s", buf->repr().c_str())) {
}

BufferOverflow::~BufferOverflow() {
}

///////////////////////////////////////////////////////////////////////////////

ByteBuffer::ByteBuffer(int capacity, char* buffer) :
    pos(0),
    markpos(-1),
    limit(capacity),
    capacity(capacity),
    own_memory(buffer == nullptr) {

    if (buffer == nullptr) {
        this->buf = (char*) malloc(capacity);
    } else {
        this->buf = buffer;
    }
}

ByteBuffer::~ByteBuffer() {
    if (this->own_memory) {
        free(this->buf);
    }
}

void ByteBuffer::compact() {
    // compact the pos to zero, and moves any memory around

    // safer
    int remaining = this->remaining();
    if (remaining == 0) {
        this->clear();
        return;
    }

    // need to compact?
    if (this->pos) {
        int pos = this->pos;

        // overlap?
        if (remaining < pos) {
            std::memcpy(this->buf, this->buf + pos, remaining);
        } else {
            std::memmove(this->buf, this->buf + pos, remaining);
        }
    }

    this->pos = remaining;
    this->limit = this->capacity;
}

string ByteBuffer::repr() const {
    return K273::fmtString("String %xd: remaining: %d, pos: %d, limit: %d",
                           this,
                           this->remaining(),
                           this->pos,
                           this->limit);
}
