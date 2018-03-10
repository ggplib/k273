include $(K273_PATH)/src/cpp/Makefile.in

SRCS += exception.cpp logging.cpp util.cpp strutils.cpp stacktrace.cpp json.cpp
SRCS += inplist.cpp parseargs.cpp

OBJS = $(patsubst %.cpp, %.o, $(SRCS))
DEPS = $(SRCS:.cpp=.d)

LIB_NAME = libk273.so

# Top level
all: $(OBJS) libk273.so

# Compiles
$(LIB_NAME): $(OBJS)
	$(CPP) -shared $(CORE_OBJS) $(LDFLAGS) $(OBJS) -pthread -lrt -ljsoncpp -o $@

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
