include $(K273_PATH)/src/cpp/Makefile.in

SRCS = greenlet.cpp
CORE_OBJS = $(patsubst %.cpp, %.o, $(SRCS))
OBJS := $(CORE_OBJS) greenlet-asm.o
DEPS=$(SRCS:.cpp=.d)

LIB_NAME = libk273_greenlet.so

# Top level
all: $(OBJS) $(LIB_NAME)

# Compiles

$(LIB_NAME): $(OBJS)
	$(CPP) -shared $(OBJS) $(LDFLAGS) -o $@

greenlet-asm.o: greenlet-asm.S
	gcc -fPIC -c -o $@ $<

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

