// local includes
#include "kelvin/selector.h"

// k273 includes
#include <k273/logging.h>
#include <k273/strutils.h>

// std includes
#include <cstring>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }

///////////////////////////////////////////////////////////////////////////////

SelectorError::SelectorError(const string &msg) :
    Exception(msg) {
}

SelectorError::~SelectorError() {
}

SelectionKey::SelectionKey(int fd, int ops, Selector* selector, void* ptdata) :
    fd(fd),
    readyops(0),
    canceled(false),
    selector(selector),
    ptdata(ptdata) {
    ASSERT (ops != 0);
    this->ops = ops;
}

SelectionKey::~SelectionKey() {
}

bool SelectionKey::valid() {
    return this->selector != nullptr;
}

int SelectionKey::getOps() const {
    return this->ops;
}

void SelectionKey::cancel() {
    ASSERT(this->valid());
    TRACE("SelectionKey::cancel(): %s", this->repr().c_str());
    if (!this->canceled) {
        this->ops = 0;
        this->canceled = true;
        this->selector->cancelIncrement();
    }
}

void SelectionKey::setOps(int ops) {
    ASSERT(this->valid());
    ASSERT(ops != 0);

    if (ops != this->ops) {
        this->ops = ops;
        this->selector->updateSelectionKey(this);
        if (this->canceled) {
            this->selector->cancelDecrement();
            this->canceled = false;
        }
    }
}

void SelectionKey::addOps(int newops) {
    this->setOps(this->ops | newops);
}

void SelectionKey::removeOps(int newops) {
    this->setOps(this->ops & ~newops);
}

void SelectionKey::reset() {
    ASSERT(this->valid());
    this->readyops = 0;
}

int SelectionKey::fileno() const {
    return this->fd;
}

void* SelectionKey::getData() const {
    return this->ptdata;
}

string SelectionKey::repr() const {
    return K273::fmtString("SelectionKey(fd=%d, ops=%d, readyops=%d, canceled=%d)",
                           this->fd,
                           this->ops,
                           this->readyops,
                           this->canceled);
}

///////////////////////////////////////////////////////////////////////////////

Selector::Selector() :
    number_keys(0),
    canceled_count(0) {
    std::memset(this->keys, 0, sizeof(SelectionKey*) * SELECTOR_MAX_FDS);
    std::memset(this->ready, 0, sizeof(SelectionKey*) * SELECTOR_MAX_FDS);
}

Selector::~Selector() {
}

int Selector::getKeyCount() {
    return this->number_keys;
}

SelectionKey** Selector::getReadyKeys() {
    return this->ready;
}

void Selector::cancelIncrement() {
    this->canceled_count++;
    TRACE("PollSelector::cancelIncrement() %d", this->canceled_count);
}

void Selector::cancelDecrement() {
    this->canceled_count--;
    TRACE("PollSelector::cancelDecrement() %d", this->canceled_count);
}

void Selector::removeCanceled() {
    ASSERT (this->canceled_count > 0);

    int found = 0;
    SelectionKey** prev = nullptr;
    for (int ii=0; ii<this->number_keys; ii++) {
        SelectionKey* ptkey = this->keys[ii];
        if (ptkey->canceled) {
            TRACE("PollSelector::removeCanceled() key: %s pos: %d",
                  ptkey->repr().c_str(), ii);

            this->finalizeInterest(ptkey);

            ptkey->canceled = false;
            ptkey->selector = nullptr;

            delete ptkey;

            if (prev == nullptr) {
                prev = &this->keys[ii];
            }

            found++;

        } else {
            // compacting up
            if (prev != nullptr) {
                *prev = ptkey;
                prev++;
            }
        }
    }

    ASSERT (found == this->canceled_count);
    this->canceled_count = 0;
    this->number_keys -= found;

    this->updateSelectionKeys();
}
