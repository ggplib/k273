
// local includes
#include "kelvin/streamer.h"
#include "kelvin/socket.h"
#include "kelvin/selector.h"
#include "kelvin/scheduler.h"
#include "kelvin/bytebuffer.h"

// k273 includes
#include <k273/logging.h>
#include <k273/strutils.h>

// std includes
#include <errno.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;
using namespace K273::Streamer;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }
///////////////////////////////////////////////////////////////////////////////

StreamHandler::StreamHandler(Scheduler* scheduler, StreamProtocol* protocol) :
    key(nullptr),
    scheduler(scheduler),
    protocol(protocol),
    // XXX how dow we make these more configurable?
    inbuf(1024 * 128),
    outbuf(1024 * 128),
    timeout_secs(0),
    write_waiting_for_os(false),
    last_update_read_time_at(0.0),
    read_timeout_cb(scheduler, this),
    sock(nullptr) {
    TRACE("creating StreamHandler %p", this);
}

StreamHandler::~StreamHandler() {
    //if (this->isConnected()) {
    //    this->disconnected();
    //}

    this->read_timeout_cb.cancel();

    TRACE("destroying StreamHandler %p", this);
}

void StreamHandler::doRead(SelectionKey* key) {
    TRACE("StreamHandler::doRead()");
    ASSERT (this->isConnected());

    int count = 0;
    char* ptbuf = this->inbuf.getInternalBuf();

    try {
        count = this->sock->recv(ptbuf, this->inbuf.remaining());

    } catch (const K273::SocketError& exc) {
        K273::l_info("Error reading from socket (fd=%d) in %s :\n  %s",
               this->sock->fileno(), this->protocol->repr().c_str(),
               exc.getMessage().c_str());

        this->disconnected();
        return;
    }

    if (count <= 0) {
        K273::l_debug("Zero length read from socket (fd=%d) in %s [count=%d, errno=%d: '%s']",
                      this->sock->fileno(), this->protocol->repr().c_str(),
                      count, errno, strerror(errno));
        this->disconnected();
        return;
    }

    this->inbuf.skip(count);

    this->updateReadTimeout();
    this->protocol->dataReceived(this->inbuf);
}

void StreamHandler::doWrite(SelectionKey* key) {
    TRACE("StreamHandler::doWrite()");
    ASSERT (this->isConnected());

    // XXX why test this flag???  Shouldn't it be an assert? XXX
    if (this->write_waiting_for_os) {

        // XXX not sure why we have to removeOps also - shouldn't selector do this?
        this->key->removeOps(OP_WRITE);
        this->write_waiting_for_os = false;

        this->outbuf.flip();
        if (this->outbuf.remaining() == 0) {
            this->outbuf.clear();
            return;
        }

        int count = 0;
        try {
            count = this->sock->send(this->outbuf.getInternalBuf(),
                                     this->outbuf.remaining());

        } catch (const K273::SocketError& exc) {
            K273::l_warning("Error writing to socket (fd=%d) in %s :\n  %s",
                            this->sock->fileno(), this->protocol->repr().c_str(),
                            exc.getMessage().c_str());
            this->disconnected();
            return;
        }

        if (count < this->outbuf.remaining()) {
            // re-register read interest with request socket
            this->key->addOps(OP_WRITE);
            this->write_waiting_for_os = true;
        }

        if (count <= 0) {
            this->outbuf.compact();
            return;
        }

        ASSERT (count <= this->outbuf.remaining());
        this->outbuf.skip(count);
        this->outbuf.compact();
    }
}

///////////////////////////////////////////////////////////////////////////////

bool StreamHandler::isConnected() {
    ASSERT ("Not implemented - should never be called.  The compiler should forbid it.");
    return false;
}

void StreamHandler::doReadTimeout() {
    ASSERT (this->isConnected());
    K273::l_warning("Read timeout on socket (fd=%d) in %s",
                    this->sock->fileno(), this->protocol->repr().c_str());
    this->disconnected();
}

std::string StreamHandler::repr() const {
    if (this->sock != nullptr) {
        return K273::fmtString("StreamHandler(fd=%d)", this->sock->fileno());
    } else {
        return K273::fmtString("StreamHandler(null)");
    }
}

void StreamHandler::disconnect() {
    // force a disconnection to happen
    this->sock->shutdown(false);
    this->sock->close(false);
    //this->disconnected();
}

void StreamHandler::write(const char* data, int size) {
    ASSERT (this->isConnected());

    int written_count = 0;
    if (!this->write_waiting_for_os) {
        try {
            written_count = this->sock->send(data, size);

        } catch (const K273::SocketError& exc) {
            K273::l_warning("Error writing to socket (fd=%d) in %s :\n  %s",
                            this->sock->fileno(), this->protocol->repr().c_str(),
                            exc.getMessage().c_str());
            this->disconnected();
            return;
        }

        // good job?
        if (written_count == size) {
            return;
        }

        // XXX tmp
        ASSERT (written_count < size);

        // XXX handle overrunning buffers...
        // append to write buffer
        written_count = std::max(0, written_count);
        this->outbuf.write(data + written_count, size - written_count);

        // register read interest with request socket
        this->key->addOps(OP_WRITE);
        this->write_waiting_for_os = true;
    }
}

void StreamHandler::setReadTimeout(int timeout_secs) {
    TRACE("Setting timeout to %d", timeout_secs);
    this->timeout_secs = timeout_secs;
    if (this->isConnected()) {
        this->updateReadTimeout();
    }
}

ConnectedSocket* StreamHandler::getSocket() {
    ASSERT (this->isConnected());
    return this->sock;
}

void StreamHandler::connected() {
    ASSERT ("Not implemented - should never be called.  The compiler should forbid it.");
}

void StreamHandler::disconnected() {
    ASSERT ("Not implemented - should never be called.  The compiler should forbid it.");
}

void StreamHandler::updateReadTimeout() {
    ASSERT (this->isConnected());

    // cancel the timer if exists
    this->read_timeout_cb.cancel();

    if (this->timeout_secs > 0) {

        // fudge it, so we don't recreate this timer excessive
        double current_time = get_time();
        if (current_time < this->last_update_read_time_at + 0.5) {
            return;
        }

        this->last_update_read_time_at = current_time;

        TRACE("updateReadTimeout");
        this->read_timeout_cb.callLater(this->timeout_secs * 1000);
    }
}

void StreamHandler::cleanupSocket() {
    ASSERT (this->sock != nullptr);

    // cleanup socket
    this->sock->shutdown(false);
    this->sock->close(false);
    delete this->sock;
    this->sock = nullptr;
}

///////////////////////////////////////////////////////////////////////////////

StreamProtocol::StreamProtocol(Scheduler* scheduler, StreamHandler* handler) :
    scheduler(scheduler),
    handler(handler),
    logically_connected(false) {
    TRACE("created StreamProtocol %p with handler %p", this, this->handler);
}

StreamProtocol::~StreamProtocol() {
    delete handler;
    TRACE("created StreamProtocol %p with handler %p", this, this->handler);
}

void StreamProtocol::dataReceived(ByteBuffer& buf) {
    /* default implementation just flips, pass buffer on to be consumed,
       compact anything remaining */
    buf.flip();
    this->onBuffer(buf);
    buf.compact();
}

void StreamProtocol::onBuffer(ByteBuffer& buf) {
    K273::l_warning("stubbed - please implement: StreamProtocol::onBuffer() %s", this->repr().c_str());
}

void StreamProtocol::connectionMade() {
    K273::l_warning("stubbed - please implement: StreamProtocol::connectionMade() %s", this->repr().c_str());
}

void StreamProtocol::connectionLost() {
    K273::l_warning("stubbed - please implement: StreamProtocol::connectionLost() %s", this->repr().c_str());
}

string StreamProtocol::repr() const {
    return "StreamProtocol";
}

void StreamProtocol::disconnect() {
    // doesn't care if we are logically connected or not
    if (this->handler->isConnected()) {
        this->handler->disconnect();
        //this->logically_connected = false;
    }
}

bool StreamProtocol::isConnected() {
    return this->handler->isConnected(); // && this->logically_connected
}

void StreamProtocol::write(const char* ptdata, int size) {
    if (this->isConnected()) {
        this->handler->write(ptdata, size);
    } else {
        K273::l_warning("call to write being dropped - but not connected : %s", this->repr().c_str());
    }
}

void StreamProtocol::write(ByteBuffer& buf) {
    if (this->isConnected()) {
        this->handler->write(buf.getInternalBuf(), buf.remaining());
        buf.compact();
    } else {
        K273::l_warning("call to write being dropped - but not connected : %s", this->repr().c_str());
    }
}

void StreamProtocol::setReadTimeout(int timeout_secs) {
    this->handler->setReadTimeout(timeout_secs);
}

ConnectedSocket* StreamProtocol::getSocket() {
    if (this->handler->isConnected()) {
        return this->handler->getSocket();
    } else {
        return nullptr;
    }
}

