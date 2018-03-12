include $(K273_PATH)/src/cpp/Makefile.in

LIBS = -L $(K273_PATH)/src/cpp/k273 -lk273

INCLUDE_PATHS += -I $(K273_PATH)/3rd/cpp -I $(K273_PATH)/3rd/range-v3/include

BINS = inplist_test.bin exception_test.bin strutils_test.bin
BINS += range_test.bin
OBJS := $(BINS:.bin=.o)
DEPS = $(OBJS:.o=.d)

# Top level
all: $(OBJS) $(BINS)

# Compiles
%.bin: $(OBJS)
	$(CPP) $(LDFLAGS) $*.o $(LIBS) -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS) -c -o $@ $<

# Cleans
clean :
	$(RM) $(BINS) $(OBJS) $(DEPS)

-include $(DEPS)
.PHONY: all clean
