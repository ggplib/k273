
include $(K273_PATH)/src/cpp/Makefile.in

SRCS += bytebuffer.cpp sharedmem.cpp socket.cpp
SRCS += selector.cpp selector_poll.cpp selector_epoll.cpp scheduler.cpp
SRCS += streamer.cpp streamer_client.cpp streamer_server.cpp

CORE_OBJS = $(patsubst %.cpp, %.o, $(SRCS))
OBJS := $(CORE_OBJS)
DEPS=$(SRCS:.cpp=.d)

# Top level
all: $(OBJS) libk273_kelvin.so

fresh : clean all

# Compiles
libk273_kelvin.so: $(OBJS)
	$(CPP) -shared $(CORE_OBJS) $(LDFLAGS) -L $(LIB_PATH) -pthread -lrt -ljsoncpp -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS_FORLIBS) -c -o $@ $<

# Cleans
clean :
	$(RM) $(OBJS) $(DEPS)

realclean : clean
	$(RM) *.so *.log core*

-include $(DEPS)
.PHONY: all fresh clean realclean


