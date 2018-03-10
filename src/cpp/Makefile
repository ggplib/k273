# Top level makefile.

all: build

fresh : clean build

clean: clean-libs
build : libs

# Faster builds.
MAKE = nice make
BUILD_CORES=-j4

all: libs

libs:
	echo "*** libs"
	$(MAKE) -C k273 $(BUILD_CORES) all
	$(MAKE) -C kelvin $(BUILD_CORES) all
	$(MAKE) -C msgq $(BUILD_CORES) all


clean-libs:
	echo "*** clean-libs"
	$(MAKE) -C k273 clean
	$(MAKE) -C kelvin clean
	$(MAKE) -C msgq clean


realclean-libs:
	echo "*** realclean-libs"
	$(MAKE) -C k273 realclean
	$(MAKE) -C kelvin realclean
	$(MAKE) -C msgq realclean


