// local includes
#include "kelvin/selector.h"

// std includes
#include <poll.h>

namespace K273 {

    class PollSelector : public Selector {
      public:
        PollSelector();
        virtual ~PollSelector();

      public:
        virtual SelectionKey* registerInterests(int fd, int ops, void* ptdata=nullptr);
        virtual int doSelect(int timeout_msecs);

      private:
        virtual void updateSelectionKey(SelectionKey* ptkey);
        virtual void updateSelectionKeys();
        virtual void finalizeInterest(SelectionKey* ptkey);

        void updatePollFd(struct pollfd* ptpollfd, SelectionKey* ptkey);

      private:
        struct pollfd pollfds[SELECTOR_MAX_FDS];
    };

}
