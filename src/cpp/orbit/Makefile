include $(K273_PATH)/src/cpp/Makefile.in

SRCS += connector.cpp other.cpp
OBJS := $(patsubst %.cpp, %.o, $(SRCS))
DEPS = $(SRCS:.cpp=.d)

OBJS := $(patsubst %.cpp, %.o, $(SRCS))
DEPS = $(SRCS:.cpp=.d)

LIB_NAME = libk273_orbit.so

# Top level
all: $(OBJS) $(LIB_NAME)

# Compiles
$(LIB_NAME): $(OBJS)
	$(CPP) -shared $(CORE_OBJS) $(LDFLAGS) -L $(LIB_PATH) -pthread -lrt -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS_FORLIBS) -c -o $@ $<

# Install
install: $(LIB_NAME)
	$(CP) $(LIB_NAME) $(INSTALL_LIB_PATH)

# Cleans
clean :
	$(RM) $(OBJS) $(DEPS) $(LIB_NAME)

-include $(DEPS)
.PHONY: all clean
