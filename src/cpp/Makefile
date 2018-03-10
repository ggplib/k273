# Top level makefile

include $(K273_PATH)/src/cpp/Makefile.in

# Faster builds.
MAKE = nice make
BUILD_CORES=-j4

all: libs

fresh : clean all install

libs:
	echo "*** libs"
	$(MAKE) -C k273 $(BUILD_CORES) all
	$(MAKE) -C kelvin $(BUILD_CORES) all
	$(MAKE) -C orbit $(BUILD_CORES) all

install: libs
	echo "*** install"
	$(MKDIR) $(INSTALL_LIB_PATH)
	$(MAKE) -C k273 install
	$(MAKE) -C kelvin install
	$(MAKE) -C orbit install

clean:
	echo "*** clean-libs"
	$(MAKE) -C k273 clean
	$(MAKE) -C kelvin clean
	$(MAKE) -C orbit clean


