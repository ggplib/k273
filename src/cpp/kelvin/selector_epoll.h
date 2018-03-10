// local includes
#include "kelvin/selector.h"

// std includes
#include <sys/epoll.h>

namespace Kelvin {

    class EPollSelector : public Selector {
      public:
        EPollSelector();
        virtual ~EPollSelector();

      public:
        virtual SelectionKey* registerInterests(int fd, int ops, void* ptdata=nullptr);
        virtual int doSelect(int timeout_msecs);

      private:
        virtual void updateSelectionKey(SelectionKey* ptkey);
        virtual void updateSelectionKeys();
        virtual void finalizeInterest(SelectionKey* ptkey);
        void updateEpollEvent(SelectionKey* ptkey, bool add_flag);

      private:
        int epoll_fd;
        struct epoll_event epoll_events[SELECTOR_MAX_FDS];
    };

}
