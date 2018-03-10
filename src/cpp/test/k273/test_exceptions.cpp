
#include <kelvin.h>
#include <kelvin/main.h>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////

const bool dumpStats = false;

///////////////////////////////////////////////////////////////////////////////

void test_throwing1() {
    /* Tests all exceptions */

    for (int ii=0; ii<10; ii++) {

        try {
            switch (ii) {
                case 0:
                    // LEAVE commented out!
                    // Shouldn't compile
                case 1:
                    throw Exception("Description here 1");
                case 2:
                    throw Exception(String("Description here 2"));
                case 3:
                    throw SysException("Description here 3", 3);
                case 4:
                    throw SysException(String("Description here 4"), 4);
                case 5:
                    throw Assertion(5, "File 5", "5 == 5");
                case 6:
                    throw Assertion(6, "File 6", "6 == 6", "Description here 6");
                case 7:
                    ASSERT(false);
                case 8:
                    ASSERT_MSG(false, "Description here 8");
                case 9 :
                default:
                    throw "asdas";
            }

        } catch (const Assertion &exc) {
            std::cerr << ii << " Assertion &exc " << exc.getMessage().getBuf() << std::endl;
            std::cerr << "  File = " << exc.getFile().getBuf() << std::endl;
            std::cerr << "  Line = " << exc.getLine() << std::endl;

        } catch (const SysException &exc) {
            std::cerr << ii << " SysException &exc " << exc.getMessage().getBuf() << std::endl;
            std::cerr << "  Get error code = " << exc.getErrorCode() << std::endl;
            std::cerr << "  Get error string = " << exc.getErrorString().getBuf() << std::endl;

        } catch (const Exception &exc) {
            std::cerr << ii << " Exception &exc " << exc.getMessage().getBuf() << std::endl;

        } catch (...) {
            std::cerr << ii << " Unknown exception caught." << std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void test_throwing2() {
    /* Tests exceptions are caught by base */

    for (int ii=0; ii<10; ii++) {

        try {
            switch (ii) {
                case 0:
                    // LEAVE commented out!
                case 1:
                    throw Exception("Description here 1");
                case 2:
                    throw Exception(String("Description here 2"));
                case 3:
                    throw SysException("Description here 3", 3);
                case 4:
                    throw SysException(String("Description here 4"), 4);
                case 5:
                    throw Assertion(5, "File 5", "5 == 5");
                case 6:
                    throw Assertion(6, "File 6", "6 == 6", "Description here 6");
                case 7:
                    ASSERT(false);
                case 8:
                    ASSERT_MSG(false, "Description here 8");
                case 9 :
                default:
                    throw "asdas";
            }

        } catch (const Exception &exc) {
            std::cerr << ii << " Exception &exc " << exc.getMessage().getBuf() << std::endl;

        } catch (...) {
            std::cerr << ii << " Unknown exception caught." << std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void test_asserting() {
    /* Tests assertions */

    for (int ii = 0; ii < 16; ii++) {

        try {
            switch (ii) {
                case 0:
                    ASSERT(true);
                    continue;
                case 1:
                    ASSERT(false);
                    continue;
                case 2:
                    ASSERT(ii == 2);
                    continue;
                case 3:
                    ASSERT(ii != 3);
                    continue;
                case 4 :
                    ASSERT_MSG(true, "Description here 4");
                    continue;
                case 5 :
                    ASSERT_MSG(false, "Description here 5");
                    continue;
                case 6 :
                    ASSERT_MSG(ii == 6, "Description here 6");
                    continue;
                case 7 :
                    ASSERT_MSG(ii == 8, "Description here 7");
                    continue;
                case 8 :
                    ASSERT(ii == 8);
                    continue;
                case 9 :
                    ASSERT(ii == 10);
                    continue;
                case 10 :
                    ASSERT('7' == '7');
                    continue;
                case 11 :
                    ASSERT('7' == 7);
                    continue;
                case 12 :
                    ASSERT(12L == 12L);
                    continue;
                case 13 :
                    ASSERT_MSG(8L == 9L, "Description here 13");
                    continue;
                default:
                    throw "asdas";
            }
        } catch (const Assertion &exc) {
            std::cerr << ii << " Assertion &exc " << exc.getMessage().getBuf() << std::endl;

        } catch (...) {
            std::cerr << "Unknown exception caught." << std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

#define TEST(name)                              \
    std::cerr << " --> test_" << #name << "()... ";  \
    test_##name();                              \
    std::cerr << "done" << std::endl;

void Main(const Strings &args) {
    initializeDev(args);
    TEST(throwing1);
    TEST(throwing2);
    TEST(asserting);
}
