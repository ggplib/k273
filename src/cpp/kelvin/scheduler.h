#pragma once

// local includes
#include "kelvin/selector.h"

// std includes
#include <queue>
#include <vector>
#include <cstdint>

namespace K273 {

    ///////////////////////////////////////////////////////////////////////////
    // Forwards

    class Deferred;
    class Scheduler;

    ///////////////////////////////////////////////////////////////////////////

    /// Timers are used internally within the scheduler to callback a Deferred object.  It is
    /// mainly an implementation detail, and not a public class.
    class Timer {
    public:
        Timer(Deferred* deferred, unsigned int priority);
        ~Timer();

    public:
        /// Called from deferred.  Cancels the timer from the scheduler.  Note Timer must be in a valid state (ie in the scheduler, and set with a
        /// deferred).
        void cancel() {
            this->deferred = nullptr;
        }

        std::string repr() const;

    private:
        /// the deferred associated with this Timer, will be null if not associated.
        Deferred* deferred;

        /// this timer in the scheduled queue (and cannot be reused by a deferred)
        bool scheduled;

        /// priority for callLater(0)s
        const unsigned int priority;

        /// time to trigger at (for callLater non zeros)
        uint64_t trigger_at_time;

        /// is a single linked list - for use in scheduler.call_laters
        Timer* next_timer;

    public:
        struct greaterthan {
            bool operator() (const Timer* t1, const Timer* t2) const {
                return t1->trigger_at_time > t2->trigger_at_time;
            }
        };

    private:
        friend class Scheduler;
    };

    ///////////////////////////////////////////////////////////////////////////

    /// deferred are objects that can be called later.  The scheduler will use
    /// the timer associated to call onWakeup() after some time in msecs.
    /// For callLaters of 0, we can set the priority in the scheduler, so that
    /// a higher priority deferred will be called back before a less priority
    /// deferred.

    class Deferred {
    public:
        Deferred(Scheduler* scheduler, int priority=0);
        virtual ~Deferred() = 0;

      public:
        /// abstract external api - should implement these different types of defers
        virtual void onWakeup() = 0;

        /// Only used for debugging purposes
        virtual std::string repr() const;

        // standard api

        /// XXX use api, needs documented
        void callLater(int msecs, bool reset=false);

        /// XXX use api, needs documented
        void cancel();

        /// XXX use api, needs documented
        bool active();

    private:
        /// private, creates and setups a timer to be scheduled.
        Timer* getTimer();

        /// Internal method that is only called from scheduled.  Ultimately calls onWakeup().
        void onWakeupFromScheduler();

    private:

        /// if this is set, it means that we have actively registered with the scheduler to be called back later.  Otherwise null.
        Timer* timer;

        /// the priority of this deferred
        const unsigned int priority;

        /// We need a reference to scheduler.
        Scheduler* scheduler;

        /// only to keep onWakeupFromScheduler() private from clients
        friend class Scheduler;
    };

    ///////////////////////////////////////////////////////////////////////////

    class EventHandler {
    public:
        EventHandler();
        virtual ~EventHandler() = 0;

    public:
        // external api interface - only need to implement them if register for the events
        virtual void doRead(SelectionKey* key);
        virtual void doWrite(SelectionKey* key);
        virtual void doAccept(SelectionKey* key);
        virtual void doConnect(SelectionKey* key);

        // must override this - is a great way to know if a corrupt object is still in the
        // scheduler somehow
        virtual std::string repr() const = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class InterruptHandler : public EventHandler {
        /* special handler to catch interrupt signals.  automatically installed. */

      public:
        explicit InterruptHandler(Scheduler* scheduler);
        virtual ~InterruptHandler();

      public:
        virtual void doRead(SelectionKey* key);
        virtual void interruptRequest(int signo);

        virtual std::string repr() const;

      public:
        int sfd;

      protected:
        sigset_t sset;
        Scheduler* scheduler;
    };

    ///////////////////////////////////////////////////////////////////////////

    class Scheduler {
    private:
        typedef std::priority_queue <Timer*, std::vector <Timer*>, Timer::greaterthan > TimerHeap;

    public:
        Scheduler(Selector* selector);
        ~Scheduler();

    public:

        // creation time
        void setInterruptHandler(InterruptHandler* handler=nullptr);

        ///////////////////////////////////
        // minimal interface to scheduler
        ///////////////////////////////////

        /*
          A selection key is returned.  Note there can only be one selection
          key per file descriptor (see Selector::registerInterests()).  Note the
          same allocation rules apply here.

          Callbacks to the handler include the registered selection key,
          thereby easy to cancel.

          A call to registerHandler() with interests of zero will cancel if a
          registration exists, or do nothing if doesn't exist.
        */

        SelectionKey* registerHandler(EventHandler* handler, int fd, int interests);

        void run(bool polling_mode=false);
        int poll(int timeout_msecs=0);
        void shutdown();

        // a valid timer must be passed in (ie it must not already be scheduled).
        void callLater(int msecs, Timer* timer);

        bool callScheduleLaters() {
            if (this->call_laters != nullptr || !this->timers.empty()) {
                this->scheduleLaters();
            }

            return this->running;
        }

    private:
        void scheduleLatersZero();
        int scheduleLaters();

        void mainLoop();

    private:
        bool running;

        InterruptHandler* interrupt_handler;

        Selector* selector;
        unsigned long long last_select_time;

        TimerHeap timers;

        // this is a single linked list
        Timer* call_laters;
    };
}


// nice macro to save some typing...

#define DEFERRED(classname, class_t, fn, args...)                       \
                                                                        \
class classname : public K273::Deferred {                               \
public:                                                                 \
    classname(K273::Scheduler* scheduler, class_t* parent) :            \
        Deferred(scheduler, ## args),                                   \
        parent(parent) {                                                \
    }                                                                   \
                                                                        \
    virtual void onWakeup() {                                           \
        this->parent->fn();                                             \
    }                                                                   \
                                                                        \
    virtual std::string repr() const {                                  \
        return #classname;                                              \
    }                                                                   \
                                                                        \
private:                                                                \
    class_t* parent;                                                    \
};
