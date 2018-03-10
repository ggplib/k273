# Top level makefile.

# Faster builds.
MAKE = nice make
BUILD_CORES=-j4

all: libs

fresh : clean all

libs:
	echo "*** libs"
	$(MAKE) -C k273 $(BUILD_CORES) all
	$(MAKE) -C kelvin $(BUILD_CORES) all
	$(MAKE) -C msgq $(BUILD_CORES) all
	$(MAKE) -C orbit $(BUILD_CORES) all


clean:
	echo "*** clean-libs"
	$(MAKE) -C k273 clean
	$(MAKE) -C kelvin clean
	$(MAKE) -C msgq clean
	$(MAKE) -C orbit clean


realclean:
	echo "*** realclean-libs"
	$(MAKE) -C k273 realclean
	$(MAKE) -C kelvin realclean
	$(MAKE) -C msgq realclean
	$(MAKE) -C orbit realclean


