#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <kelvin/main.h>
#include <kelvin/strutils.h>


#define TEST(name)                                   \
    std::cerr << " --> test_" << #name << "()... ";  \
    test_##name();                                   \
    std::cerr << "done" << std::endl;


void test_listdir_basic_1() {
    using namespace KelvinLib;

    Strings files = listdir("/tmp");

    ASSERT(files.size() > 0);

    std::cout << "OK" << std::endl;
}


void test_listdir_basic_2() {
    using namespace KelvinLib;

    const pid_t pid = ::getpid();
    const String path = fmtString("/proc/%d/fd", pid);

    const Strings files = listdir(path);
    ASSERT(std::strcmp(files.at(0).getBuf(), ".") == 0);
    ASSERT(std::strcmp(files.at(1).getBuf(), "..") == 0);

    int i = 0;
    for (auto itr = files.cbegin() + 2; itr != files.cend(); ++itr) {
        ASSERT(std::strcmp(itr->getBuf(), fmtString("%d", i).getBuf()) == 0);
        ++i;
    }

    // ., .., 0, 1, 2
    ASSERT(files.size() >= 5);

    std::cout << "OK" << std::endl;
}


void test_listdir_nopermission_1() {
    using namespace KelvinLib;

    try {
        Strings files = listdir("/root");
        ASSERT(false);
    } catch (const SysException& e) {
        std::cout << "OK" << std::endl;
    }
}


void test_listdir_nonexisting_dir_1() {
    using namespace KelvinLib;

    try {
        Strings files = listdir("/asjfasdf");
        ASSERT(false);
    } catch (const SysException& e) {
        std::cout << "OK" << std::endl;
    }
}


void Main(const KelvinLib::Strings &args) {
    initializeDev(args);

    TEST(listdir_basic_1);
    TEST(listdir_basic_2);
    TEST(listdir_nopermission_1);
    TEST(listdir_nonexisting_dir_1);
}
