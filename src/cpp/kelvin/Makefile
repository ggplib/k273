include $(K273_PATH)/src/cpp/Makefile.in

SRCS += bytebuffer.cpp sharedmem.cpp socket.cpp
SRCS += selector.cpp selector_poll.cpp selector_epoll.cpp scheduler.cpp
SRCS += streamer.cpp streamer_client.cpp streamer_server.cpp
SRCS += msgq/other.cpp

OBJS := $(patsubst %.cpp, %.o, $(SRCS))
DEPS = $(SRCS:.cpp=.d)

LIB_NAME = libk273_kelvin.so

# Top level
all: $(OBJS) $(LIB_NAME)

# Compiles
$(LIB_NAME): $(OBJS)
	$(CPP) -shared $(CORE_OBJS) $(LDFLAGS) -L $(LIB_PATH) -pthread -lrt -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS_FORLIBS) -c -o $@ $<

# Install
install:
	$(CP) $(LIB_NAME) $(INSTALL_LIB_PATH)

# Cleans
clean :
	$(RM) $(OBJS) $(DEPS) $(LIB_NAME)

-include $(DEPS)
.PHONY: all clean
