#pragma once

// k273 includes
#include <k273/exception.h>

///////////////////////////////////////////////////////////////////////////////

#define OP_NONE 0
#define OP_ACCEPT (1 << 0)
#define OP_CONNECT (1 << 1)
#define OP_READ (1 << 2)
#define OP_WRITE (1 << 3)

#define KEY_READ (OP_ACCEPT | OP_READ)
#define KEY_WRITE (OP_CONNECT | OP_WRITE)

// XXX Why this max???
#define SELECTOR_MAX_FDS 128

namespace K273::Kelvin {

    ///////////////////////////////////////////////////////////////////////////
    // forwards

    // local
    class Selector;
    class PollSelector;
    class EPollSelector;

    // from scheduler
    class Scheduler;

    ///////////////////////////////////////////////////////////////////////////

    class SelectorError: public Exception {
      public:
        SelectorError(const std::string &msg);
        virtual ~SelectorError();
    };

    ///////////////////////////////////////////////////////////////////////////

    class SelectionKey {
    public:
        SelectionKey(int fd, int ops, Selector* selector, void* data=nullptr);
        ~SelectionKey();

      public:
        bool valid();

        int getOps() const;

        void cancel();

        // note removeOps() and setOps() should not set the ops to zero.  Use
        // cancel for this.
        void setOps(int ops);
        void addOps(int newops);
        void removeOps(int newops);

        void reset();

        int fileno() const;
        void* getData() const;

        std::string repr() const;

    private:
        int fd;
        int ops;
        int readyops;

        // so we increment canceled_count properly
        bool canceled;

        Selector* selector;

        // store pointer to arbirtary data with this selector
        void* ptdata;

    public:
        friend class Selector;
        friend class PollSelector;
        friend class EPollSelector;
        friend class Scheduler;
    };

    ///////////////////////////////////////////////////////////////////////////

    // Abstract interface class
    class Selector {
    public:
        Selector();
        virtual ~Selector();

    public:
        // getters
        int getKeyCount();
        SelectionKey** getReadyKeys();

        // subclasses must implement these two

        /* There can only be one selection key per file descriptor.
           Re-registering will over-write the ops and ptdata.  When the key is
           canceled, on the next doSelect() call the SelectionKey memory will
           be deallocated. It is possible to cancel and re-register before the
           next doSelect() and in that case the memory won't be deallocated. */
        virtual SelectionKey* registerInterests(int fd, int ops, void* ptdata=nullptr) = 0;

        // actually do the selection
        virtual int doSelect(int timeout_msecs) = 0;

      protected:
        // Update interest for a specific key
        virtual void updateSelectionKey(SelectionKey* ptkey) = 0;

        // Update interest for all keys
        virtual void updateSelectionKeys() = 0;

        // Finalize the selector and do any cleaning up for this selector
        virtual void finalizeInterest(SelectionKey* ptkey) = 0;

      protected:
        void cancelIncrement();
        void cancelDecrement();
        void removeCanceled();
        void addInterest(int fd, int ops, void* ptdata);

      protected:
        int number_keys;
        int canceled_count;
        SelectionKey* keys[SELECTOR_MAX_FDS];
        SelectionKey* ready[SELECTOR_MAX_FDS];

      public:
        friend class SelectionKey;
    };

}
