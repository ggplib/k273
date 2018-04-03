include $(K273_PATH)/src/cpp/Makefile.in

LIBS = -L $(K273_PATH)/src/cpp/k273 -lk273

# XXX add these to top level?
INCLUDE_PATHS += -I $(K273_PATH)/3rd/cpp
INCLUDE_PATHS += -I $(K273_PATH)/3rd/cpp/range-v3/include
INCLUDE_PATHS += -I $(K273_PATH)/3rd/cpp/fmt-4.1.0
#INCLUDE_PATHS += -I $(K273_PATH)/3rd/cpp/itertools

CATCH2_BIN = catch2
CATCH2_SRCS = strutils_test.cpp inplist_test.cpp catch2_runner.cpp
CATCH2_OBJS = $(patsubst %.cpp, %.o, $(CATCH2_SRCS))

OTHER_BINS = exception_test.bin
OTHER_OBJS := $(OTHER_BINS:.bin=.o)

DEPS = $(CATCH2_OBJS:.o=.d) $(OTHER_OBJS:.o=.d)

# Top level
all: $(CATCH2_BIN) $(OTHER_BINS)

# Compiles
%.bin: $(OTHER_OBJS)
	$(CPP) $(LDFLAGS) $*.o $(LIBS) -o $@

$(CATCH2_BIN): $(CATCH2_OBJS)
	$(CPP) $(LDFLAGS) $(CATCH2_OBJS) $(LIBS) -o $@

%.o : %.cpp
	$(CPP) $(INCLUDE_PATHS) $(CFLAGS) -c -o $@ $<

# Cleans
clean :
	$(RM) $(CATCH2_BIN) $(CATCH2_OBJS) $(OTHER_BINS) $(OTHER_OBJS) $(DEPS)

-include $(DEPS)
.PHONY: all clean
