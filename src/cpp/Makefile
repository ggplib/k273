# Top level makefile.

all: build

fresh : clean build

clean: clean-libs-testing clean-libs clean-all-apps
build : libs libs-testing all-apps

# Faster builds.
MAKE = nice make
BUILD_CORES=-j4

all: libs libs-testing all-apps

libs:
	echo "*** Building libs"
	$(MAKE) -C k273 $(BUILD_CORES) all
	$(MAKE) -C kelvin $(BUILD_CORES) all
	$(MAKE) -C msgq $(BUILD_CORES) all

libs-testing:
	echo "*** Building libs_testing"
	$(MAKE) -C k273_testing $(BUILD_CORES) all
	$(MAKE) -C kelvin_testing $(BUILD_CORES) all

all-apps:
	echo "*** Building apps"
	$(MAKE) -C apps/shepherd $(BUILD_CORES)

# cleans - call realclean in sub-makefiles

clean-libs:
	echo "*** Clean libs"
	$(MAKE) -C k273 realclean
	$(MAKE) -C kelvin realclean
	$(MAKE) -C msgq realclean

clean-libs-testing:
	echo "*** Clean libs-testing"
	$(MAKE) -C k273_testing realclean
	$(MAKE) -C kelvin_testing realclean

clean-all-apps:
	echo "*** Clean apps"
	$(MAKE) -C apps/shepherd realclean

