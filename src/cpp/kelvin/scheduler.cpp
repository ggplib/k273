// local includes
#include "kelvin/scheduler.h"

// k273 includes
#include <k273/logging.h>
#include <k273/strutils.h>

// std includes
#include <string>
#include <cerrno>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }

///////////////////////////////////////////////////////////////////////////////

#define SLEEP_MSECS 60 * 1000

///////////////////////////////////////////////////////////////////////////////

uint64_t msecs_time() {
    return (uint64_t) (K273::get_time() * 1000);
}
///////////////////////////////////////////////////////////////////////////////

Deferred::Deferred(Scheduler* scheduler, int a_priority) :
    timer(nullptr),
    priority(a_priority),
    scheduler(scheduler) {

    ASSERT (priority >= 0);
}

Deferred::~Deferred() {
    this->cancel();
}

string Deferred::repr() const {
    return "Deferred";
}

void Deferred::callLater(int msecs, bool reset) {
    if (reset) {
        // if reseting - we first cancel (which if active will cancel timer) and redo.
        this->cancel();
        this->scheduler->callLater(msecs, this->getTimer());

    } else {
        // if not reseting - and active, we don't do anything.
        if (!this->active()) {
            this->scheduler->callLater(msecs, this->getTimer());
        }
    }
}

void Deferred::cancel() {
    // if the timer is active, it is not null.  And it therefore must be scheduled.
    if (this->timer != nullptr) {
        this->timer->cancel();

        // the scheduler will delete the memory when the timer expires
        this->timer = nullptr;
    }
}

bool Deferred::active() {
    return this->timer != nullptr;
}

Timer* Deferred::getTimer() {
    TRACE("creating timer in %s", this->repr().c_str());
    this->timer = new Timer(this, this->priority);
    return this->timer;
}

void Deferred::onWakeupFromScheduler() {
    // we need to set this first, incase we reschedule ourselves
    this->timer = nullptr;
    this->onWakeup();
}

///////////////////////////////////////////////////////////////////////////////

EventHandler::EventHandler() {
}

EventHandler::~EventHandler() {
}

void EventHandler::doRead(SelectionKey* key) {
    ASSERT_MSG(false, K273::fmtString("%s: doRead() not implemented, shouldn't be registered", this->repr().c_str()));
}

void EventHandler::doWrite(SelectionKey* key) {
    ASSERT_MSG(false, K273::fmtString("%s: doWrite() not implemented, shouldn't be registered", this->repr().c_str()));
}

void EventHandler::doAccept(SelectionKey* key) {
    ASSERT_MSG(false, K273::fmtString("%s: doAccept() not implemented, shouldn't be registered", this->repr().c_str()));
}

void EventHandler::doConnect(SelectionKey* key) {
    ASSERT_MSG(false, K273::fmtString("%s: doConnect() not implemented, shouldn't be registered", this->repr().c_str()));
}

string EventHandler::repr() const {
    return "EventHandler";
}

///////////////////////////////////////////////////////////////////////////////

Timer::Timer(Deferred* a_deferred, unsigned int a_priority) :
    deferred(a_deferred),
    scheduled(false),
    priority(a_priority),
    trigger_at_time(-1),
    next_timer(nullptr) {
}

Timer::~Timer() {
}

string Timer::repr() const {
    if (this->deferred == nullptr) {
        return "Timer(handler=null)";

    } else if (this->trigger_at_time == 0) {
        return K273::fmtString("Timer0(handler=%s)", this->deferred->repr().c_str());

    } else {
        return K273::fmtString("Time(handler=%s, trigger_time=%d)",
                               this->deferred->repr().c_str(),
                               this->trigger_at_time);
    }
}

///////////////////////////////////////////////////////////////////////////////

InterruptHandler::InterruptHandler(Scheduler* scheduler) :
    scheduler(scheduler) {
    sigemptyset(&this->sset);
    sigaddset(&this->sset, SIGINT);
    sigaddset(&this->sset, SIGTERM);
    sigprocmask(SIG_BLOCK, &this->sset, nullptr);
    if ((this->sfd = signalfd(-1, &sset, 0)) == -1) {
        throw SysException("creating signalfd()", errno);
    }
}

InterruptHandler::~InterruptHandler() {
    // XXX is this sufficient cleanup?
    close(this->sfd);
}

void InterruptHandler::doRead(SelectionKey* key) {
    ASSERT(key->fileno() == this->sfd);

    struct signalfd_siginfo info;

    int bytes = read(this->sfd, &info, sizeof(struct signalfd_siginfo));
    if (bytes < 0) {
        throw SysException("Failed to read from sfd", errno);
    }

    ASSERT(bytes == sizeof(struct signalfd_siginfo));

    this->interruptRequest(info.ssi_signo);
}

void InterruptHandler::interruptRequest(int signo) {
    K273::l_info("signal received (%s) - doing shutdown", strsignal(signo));
    this->scheduler->shutdown();
}

string InterruptHandler::repr() const {
    return "SignalHandler";
}

///////////////////////////////////////////////////////////////////////////////

Scheduler::Scheduler(Selector* selector) :
    running(false),
    interrupt_handler(nullptr),
    selector(selector),
    call_laters(nullptr) {

    // we need to set this at creation time incase anyone adds a callLater
    // before we start the main loop
    this->last_select_time = msecs_time();
    this->setInterruptHandler();
}

Scheduler::~Scheduler() {
    delete this->interrupt_handler;
}


void Scheduler::setInterruptHandler(InterruptHandler* handler) {
    if (this->interrupt_handler != nullptr) {
        this->registerHandler(this->interrupt_handler, this->interrupt_handler->sfd, 0);
        delete this->interrupt_handler;
    }

    if (handler == nullptr) {
        handler = new InterruptHandler(this);
    }

    this->interrupt_handler = handler;
    this->registerHandler(handler, handler->sfd, OP_READ);
}

SelectionKey* Scheduler::registerHandler(EventHandler* handler, int fd, int interests) {
    return this->selector->registerInterests(fd, interests, handler);
}

void Scheduler::callLater(int msecs, Timer* timer) {
    /* The deallocation of Timers are handled internally.

       If the msecs is 0, it will be given priority over the timed events and added to
       call_laters deque on a priority basis. */

    ASSERT(!timer->scheduled);

    timer->scheduled = true;

    if (msecs == 0) {
        timer->trigger_at_time = 0;

        Timer* prev = nullptr;
        Timer* cur = this->call_laters;
        while (true) {
            // insert before
            if (cur == nullptr || timer->priority > cur->priority) {
                timer->next_timer = cur;

                // is this the front?
                if (prev == nullptr) {
                    this->call_laters = timer;
                } else {
                    prev->next_timer = timer;
                }

                break;
            }

            prev = cur;
            cur = cur->next_timer;
        }

    } else {
        timer->trigger_at_time = this->last_select_time + msecs;
        this->timers.push(timer);
    }
}

void Scheduler::run(bool polling_mode) {
    this->running = true;
    if (!polling_mode) {
        this->mainLoop();
    }
}

void Scheduler::shutdown() {
    this->running = false;
}

///////////////////////////////////////////////////////////////////////////////

void Scheduler::scheduleLatersZero() {
    while (this->call_laters != nullptr) {

        // very useful for debugging purposes.  Note only on verbose, but not when tracing.  It is too much debug.
        if (_debug_log) {
            string s = "";
            Timer* cur = this->call_laters;
            while (cur != nullptr) {
                s += cur->repr();
                cur = cur->next_timer;
                if (cur != nullptr) {
                    s += ", ";
                }
            }

            K273::l_debug("SCHEDULER:callLaters(0) -> %s", s.c_str());
        }

        // get the head
        Timer* timer = this->call_laters;

        // take a copy of deferred, we want to reset the timer before calling wakeup
        Deferred* the_deferred = timer->deferred;

        // this effectively pops the head
        this->call_laters = timer->next_timer;

        // cleans up memory
        delete timer;

        // finally call (the null test is since a cancelled callLater() will set the_deferred to be null)
        if (the_deferred != nullptr) {
            the_deferred->onWakeupFromScheduler();
        }
    }
}

int Scheduler::scheduleLaters() {

    while (this->call_laters != nullptr || !this->timers.empty()) {

        if (this->call_laters != nullptr) {
            this->scheduleLatersZero();
            continue;
        }

        Timer* timer = this->timers.top();

        // check if the timer is still associated with the Timer, and if not we can just continue.
        if (timer->deferred == nullptr) {
            this->timers.pop();

            // cleans up memory
            delete timer;

        } else {
            const int timeout_msecs = timer->trigger_at_time - this->last_select_time;
            if (timeout_msecs > 0) {
                //l_debug("Scheduler::scheduleTimers() size: %zu", this->timers.size());
                return timeout_msecs;
            }

            this->timers.pop();

            // take a copy of deferred, we want to reset the timer before calling wakeup
            Deferred* the_deferred = timer->deferred;

            // cleans up memory
            delete timer;

            // finally call
            the_deferred->onWakeupFromScheduler();
        }
    }

    return SLEEP_MSECS;
}

int Scheduler::poll(int timeout_msecs) {

    if (!this->running) {
        return -1;
    }

    int ready_count = selector->doSelect(timeout_msecs);

    TRACE("selector->doSelect() readycount %d", ready_count);

    this->last_select_time = msecs_time();

    SelectionKey** ptptready = selector->getReadyKeys();
    for (int ii=0; ii<ready_count; ii++, ptptready++) {

        SelectionKey* key = *ptptready;
        EventHandler* handler = static_cast <EventHandler*> (key->ptdata);

        TRACE("mainLoop key: %s", key->repr().c_str());

        if (unlikely(key->canceled)) {
            TRACE("key(%s) was canceled - ignoring", handler->repr().c_str());
            key->reset();
            continue;
        }

        if (key->readyops & OP_READ) {
            TRACE("doRead(%s)", handler->repr().c_str());
            handler->doRead(key);

        } else if (unlikely(key->readyops & OP_ACCEPT)) {
            TRACE("doAccept(%s)",handler->repr().c_str());
            handler->doAccept(key);
        }

        if (key->readyops & OP_WRITE) {
            TRACE("doWrite(%s)", handler->repr().c_str());
            handler->doWrite(key);

        } else if (unlikely(key->readyops & OP_CONNECT)) {
            TRACE("doConnect(%s)", handler->repr().c_str());
            handler->doConnect(key);
        }

        // finally we can reset the key
        key->reset();
    }

    // we check the timers once we finished handling the data off the select loop
    return this->scheduleLaters();
}

void Scheduler::mainLoop() {
    this->last_select_time = msecs_time();
    int timeout_msecs = this->scheduleLaters();
    while (this->running) {
        timeout_msecs = this->poll(timeout_msecs);
    }
}
