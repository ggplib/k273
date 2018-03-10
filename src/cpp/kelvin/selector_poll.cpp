// local includes
#include "kelvin/selector.h"
#include "kelvin/selector_poll.h"

// k273 includes
#include <k273/logging.h>

// std includes
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273::Kelvin;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }

///////////////////////////////////////////////////////////////////////////////

PollSelector::PollSelector() {
    ::memset(this->pollfds, 0, sizeof(struct pollfd) * SELECTOR_MAX_FDS);
}

PollSelector::~PollSelector() {
}

SelectionKey* PollSelector::registerInterests(int fd, int ops, void* ptdata) {
    /* will register an interest for a file descriptor */

    TRACE("PollSelector::registerInterests() fd: %d, ops: %d, number_keys: %d", fd, ops, this->number_keys);

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

            ptkey->ptdata= ptdata;
            return ptkey;
        }
    }

    if (this->number_keys == SELECTOR_MAX_FDS) {
        throw SelectorError("Exceeded number of registrations");
    }

    SelectionKey* key = new SelectionKey(fd, ops, this, ptdata);
    struct pollfd* ptpollfd = &this->pollfds[this->number_keys];
    this->updatePollFd(ptpollfd, key);
    this->number_keys++;
    *ptptkey = key;

    TRACE("PollSelector::registerInterests() / new registration key: %s number_keys: %d",
          key->repr().c_str
          (), this->number_keys);

    return key;
}

int PollSelector::doSelect(int timeout_msecs) {
    TRACE("PollSelector::doSelect() timeout_msecs %d, nfds: %d ", timeout_msecs, this->number_keys);

    if (this->canceled_count) {
        this->removeCanceled();
    }

    // fake a timeoot if nothing to poll
    if (this->number_keys == 0) {
        usleep(timeout_msecs * 1000);
        return 0;
    }

    if (_debug_log) {
        TRACE("IN keys: %d", this->number_keys);
        struct pollfd* p = this->pollfds;
        for (int ii=0; ii<this->number_keys; ii++, p++) {
            TRACE("  fd: %d, events: %d, revents: %d", p->fd, p->events, p->revents);
        }
    }


    int ready_count = poll(this->pollfds, this->number_keys, timeout_msecs);
    if (ready_count < 0)  {
        if (errno == EINTR) {
            // A signal occurred before any requested events
            return 0;
        }

        throw SysException("An error occurred during poll()", errno);
    }

    if (_debug_log) {
        TRACE("OUT ready_count: %d ", ready_count);
        struct pollfd* p = this->pollfds;
        for (int ii=0; ii<this->number_keys; ii++, p++) {
            TRACE("  fd: %d, events: %d, revents: %d", p->fd, p->events, p->revents);
        }
    }

    int done = 0;

    struct pollfd* ptpollfd = this->pollfds;
    SelectionKey** ptready = this->ready;


    for (int ii=0; ii<this->number_keys; ii++, ptpollfd++) {
        if (done == ready_count) {
            break;
        }

        if (ptpollfd->revents) {

            SelectionKey* ptkey = this->keys[ii];

            if (ptpollfd->revents & POLLIN) {
                ptkey->readyops |= ptkey->getOps() & KEY_READ;
                TRACE("POLLIN %s", ptkey->repr().c_str());
            }

            if (ptpollfd->revents & POLLOUT) {
                ptkey->readyops |= ptkey->getOps() & KEY_WRITE;
                TRACE("POLLOUT %s", ptkey->repr().c_str());
            }

            if (ptpollfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
                ptkey->readyops |= ptkey->getOps() & (KEY_READ | KEY_WRITE);
                TRACE("POLLERR | POLLHUP | POLLNVAL %s", ptkey->repr().c_str());
            }

            done++;
            *ptready = ptkey;
            ptready++;
        }
    }

    ASSERT(done == ready_count);
    return ready_count;
}

///////////////////////////////////////////////////////////////////////////////

void PollSelector::updateSelectionKey(SelectionKey* ptkey) {
    struct pollfd* ptpollfd = this->pollfds;
    for (int ii=0; ii<this->number_keys; ii++, ptpollfd++) {
        if (ptkey == this->keys[ii]) {
            this->updatePollFd(ptpollfd, ptkey);
            return;
        }
    }

    ASSERT_MSG(false, "Should not get here, key does not exist");
}

void PollSelector::updateSelectionKeys() {
    struct pollfd* ptpollfd = this->pollfds;
    for (int ii=0; ii<this->number_keys; ii++, ptpollfd++) {
        SelectionKey* ptkey = this->keys[ii];
        this->updatePollFd(ptpollfd, ptkey);
    }

    TRACE("PollSelector::updateAllInterests() updated interests (number_keys : %d)",
          this->number_keys);
}

void PollSelector::finalizeInterest(SelectionKey* ptkey) {
}

void PollSelector::updatePollFd(struct pollfd* ptpollfd, SelectionKey* ptkey) {
    int flags = 0;
    if (ptkey->getOps() & KEY_READ) {
        flags = POLLIN | POLLPRI;
    }

    if (ptkey->getOps() & KEY_WRITE) {
        flags |= POLLOUT;
    }

    TRACE("PollSelector::updatePollFd() %s, number_keys: %d",
          ptkey->repr().c_str(), this->number_keys);

    ptpollfd->fd = ptkey->fd;
    ptpollfd->events = flags;
}
