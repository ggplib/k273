
include $(K273_PATH)/src/cpp/Makefile.in

SRCS += exception.cpp logging.cpp util.cpp strutils.cpp stacktrace.cpp json.cpp
SRCS += inplist.cpp parseargs.cpp

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
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS_FORLIBS) -c -o $@ $<

# Cleans
clean :
	$(RM) $(OBJS) $(DEPS)

realclean : clean
	$(RM) *.so *.log core*

-include $(DEPS)
.PHONY: all fresh clean realclean


