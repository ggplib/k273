
include $(K273_PATH)/src/cpp/Makefile.in

SRCS += other.cpp

CORE_OBJS = $(patsubst %.cpp, %.o, $(SRCS))
OBJS := $(CORE_OBJS)
DEPS=$(SRCS:.cpp=.d)

# Top level
all: $(OBJS) libk273_msgq.so

fresh : clean all

# Compiles
libk273_msgq.so: $(OBJS)
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


