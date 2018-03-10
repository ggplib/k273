
// XXX FIX&TEST THIS

// local includes
#include "kelvin/selector.h"
#include "kelvin/selector_epoll.h"

// k273 includes
#include <k273/logging.h>

// std includes
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }

///////////////////////////////////////////////////////////////////////////////

EPollSelector::EPollSelector() {
    TRACE("EPollSelector::EPollSelector()");
    this->epoll_fd = epoll_create(SELECTOR_MAX_FDS);
    memset(this->epoll_events, 0, sizeof(struct epoll_event) * SELECTOR_MAX_FDS);
}

EPollSelector::~EPollSelector() {
    close(this->epoll_fd);
}

SelectionKey* EPollSelector::registerInterests(int fd, int ops, void* ptdata) {
    /* will register an interest for a file descriptor */

    // XXX common code start
    TRACE("EPollSelector::registerInterests() fd: %d, ops: %d, number_keys: %d", fd, ops, this->number_keys);

    SelectionKey** ptptkey = this->keys;
    for (int ii=0; ii<this->number_keys; ii++, ptptkey++) {
        SelectionKey* ptkey = *ptptkey;
        if (ptkey->fd == fd) {

            // update ops and data
            if (ops) {
                ptkey->setOps(ops);
            } else {
                ptkey->cancel();
            }

            ptkey->ptdata = ptdata;
            return ptkey;
        }
    }

    if (this->number_keys == SELECTOR_MAX_FDS) {
        throw SelectorError("Exceeded number of registrations");
    }

    SelectionKey* key = new SelectionKey(fd, ops, this, ptdata);

    // XXX common code end

    this->updateEpollEvent(key, true);
    this->number_keys++;
    *ptptkey = key;

    TRACE("PollSelector::registerInterests() / new registration key: %s number_keys: %d",
          key->repr().c_str(), this->number_keys);

    return key;
}

void EPollSelector::updateSelectionKey(SelectionKey* ptkey) {
    this->updateEpollEvent(ptkey, false);
}

void EPollSelector::updateSelectionKeys() {
    for (int ii=0; ii<this->number_keys; ii++) {
        SelectionKey* ptkey = this->keys[ii];
        this->updateEpollEvent(ptkey, false);
    }

    TRACE("PollSelector::updateAllInterests() updated interests (number_keys : %d)",
          this->number_keys);
}

void EPollSelector::finalizeInterest(SelectionKey* ptkey) {
    TRACE("PollSelector::finalizeInterest() to fd: %d)", ptkey->fileno());

    int res = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, ptkey->fileno(), nullptr);
    if (res != 0) {
        if (errno == EBADF) {
            return;
        }

        throw SysException("An error occurred during epoll_ctl()", errno);
    }
}

int EPollSelector::doSelect(int timeout_msecs) {

    TRACE("EPollSelector::doSelect() timeout_msecs %d, cancel: %d, nfds: %d ",
          timeout_msecs, this->canceled_count, this->number_keys);

    if (this->canceled_count) {
        this->removeCanceled();
    }

    // fake a timeoot if nothing to poll
    if (this->number_keys == 0) {
        usleep(timeout_msecs * 1000);
        return 0;
    }

    int ready_count = epoll_wait(this->epoll_fd, this->epoll_events, this->number_keys, timeout_msecs);

    TRACE("EPollSelector::doSelect() ready_count: %d ", ready_count);

    if (ready_count < 0) {
        if (errno == EINTR) {
            // A signal occurred before any requested events
            return 0;
        }

        throw SysException("An error occurred during epoll()", errno);
    }

    ASSERT (ready_count <= SELECTOR_MAX_FDS);

    struct epoll_event* ptevent = this->epoll_events;
    SelectionKey** ptready = this->ready;

    for (int ii=0; ii<ready_count; ii++, ptevent++, ptready++) {
        SelectionKey* ptkey = (SelectionKey*) ptevent->data.ptr;

        if (ptevent->events & EPOLLIN) {
            ptkey->readyops |= ptkey->getOps() & KEY_READ;
            TRACE("POLLIN %s", ptkey->repr().c_str());
        }

        if (ptevent->events & EPOLLOUT) {
            ptkey->readyops |= ptkey->getOps() & KEY_WRITE;
            TRACE("POLLOUT %s", ptkey->repr().c_str());
        }

        if (ptevent->events & (EPOLLERR | EPOLLHUP)) {
            ptkey->readyops |= ptkey->getOps() & (KEY_READ | KEY_WRITE);
            TRACE("POLLERR | POLLHUP | POLLNVAL %s", ptkey->repr().c_str());
        }

        *ptready = ptkey;
    }

    return ready_count;
}

///////////////////////////////////////////////////////////////////////////////

void EPollSelector::updateEpollEvent(SelectionKey* ptkey, bool add_flag) {
    // XXX guessing a bit here...
    int events = EPOLLERR | EPOLLHUP | EPOLLRDHUP; // XXX?
    if (ptkey->getOps() & KEY_READ) {
        events = EPOLLIN | EPOLLPRI;
    }

    if (ptkey->getOps() & KEY_WRITE) {
        events |= EPOLLOUT;
    }

    struct epoll_event event;
    event.events = events;
    event.data.ptr = ptkey;
    int op = add_flag ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    int res = epoll_ctl(this->epoll_fd, op, ptkey->fileno(), &event);
    if (res != 0) {
        if (errno == EBADF) {
            K273::l_error("An error occurred during epoll_ctl() : fd is bad %d", ptkey->fileno());
            return;
        }

        throw SysException("An error occurred during epoll_ctl()", errno);
    }
}
