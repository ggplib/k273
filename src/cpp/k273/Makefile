
INCLUDE_PATH = -I ../ -I /usr/include/jsoncpp
LIB_PATH = ./

AR = ar
RM = rm -f
CPP = g++

CFLAGS = -MMD -MP -g -Wall -Werror -Woverloaded-virtual -Wnon-virtual-dtor -O3 -fPIC -fomit-frame-pointer -std=c++11
LDFLAGS = -g -MMD -MP -Wall -Werror -Woverloaded-virtual -Wnon-virtual-dtor -O3 -fomit-frame-pointer -std=c++11


SRCS += exception.cpp logging.cpp util.cpp strutils.cpp stacktrace.cpp json.cpp
SRCS += inplist.cpp parseargs.cpp sharedmem.cpp socket.cpp

CORE_OBJS = $(patsubst %.cpp, %.o, $(SRCS))
OBJS := $(CORE_OBJS)
DEPS=$(SRCS:.cpp=.d)

# Top level
all: $(OBJS) libk273.so

fresh : clean all

# Compiles
libk273.so: $(OBJS)
	$(CPP) -shared $(CORE_OBJS) $(LDFLAGS) -L $(LIB_PATH) -pthread -lrt -ljsoncpp -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $<

# Cleans
clean :
	$(RM) $(OBJS) $(DEPS)

realclean : clean
	$(RM) *.so *.log core*

-include $(DEPS)
.PHONY: all fresh clean realclean


