include $(K273_PATH)/src/cpp/Makefile.in

LIBS = -L $(K273_PATH)/src/cpp/k273 -lk273 -L $(K273_PATH)/src/cpp/kelvin -lk273_kelvin

BINS = client.bin server.bin
SRCS = common.cpp

CORE_OBJS = $(SRCS:.cpp=.o)
OBJS = $(CORE_OBJS) $(BINS:.bin=.o)
DEPS = $(SRCS:.cpp=.d) $(BINS:.bin=.d)

# Top level
all: $(OBJS) $(BINS)

# Compiles
%.bin: $(CORE_OBJS)
	$(CPP) $(LDFLAGS) $*.o $(CORE_OBJS) $(LIBS) -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS) -c -o $@ $<

# Cleans
clean :
	$(RM) $(BINS) $(CORE_OBJS) $(DEPS)

-include $(DEPS)
.PHONY: all clean
