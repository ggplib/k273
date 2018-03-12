//#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
//#include "catch.hpp"

// k273 includes
#include <k273/strutils.h>
#include <k273/exception.h>

// 3rd party
#include <range/v3/all.hpp>


// std includes
#include <vector>
#include <string>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>


void test_listdir_basic_1() {
    const auto files = K273::listdir("/tmp");

    ASSERT(files.size() > 0);

    std::cout << "OK" << std::endl;
}


void test_listdir_basic_2() {
    const pid_t pid = ::getpid();
    const std::string path = K273::fmtString("/proc/%d/fd", pid);

    auto files = K273::listdir(path);

    std::cout << files.at(0) << std::endl;
    std::cout << files.at(1) << std::endl;

    ASSERT(files.at(0) == ".");
    ASSERT(files.at(1) == "..");

    // drop this first 2 elements
    auto files_range = files | ranges::view::drop(2);
    int i = 0;
    for (auto f : files_range) {
        ASSERT(f == K273::fmtString("%d", i++));
    }

    // ., .., 0, 1, 2
    ASSERT(files.size() >= 5);

    std::cout << "OK" << std::endl;
}


void test_listdir_nopermission_1() {
    try {
        auto files = K273::listdir("/root");
        ASSERT(false);
    } catch (const K273::SysException& e) {
        std::cout << "OK" << std::endl;
    }
}


void test_listdir_nonexisting_dir_1() {
    try {
        auto files = K273::listdir("/asjfasdf");
        ASSERT(false);
    } catch (const K273::SysException& e) {
        std::cout << "OK" << std::endl;
    }
}


#define TEST(name)                                   \
    std::cerr << " --> test_" << #name << "()... ";  \
    test_##name();                                   \
    std::cerr << "done" << std::endl;


void run(std::vector <std::string>& args) {
    TEST(listdir_basic_1);
    TEST(listdir_basic_2);
    TEST(listdir_nopermission_1);
    TEST(listdir_nonexisting_dir_1);
}

///////////////////////////////////////////////////////////////////////////////

#include <k273/runner.h>

int main(int argc, char** argv) {
    K273::Runner::Config config(argc, argv);
    config.log_filename = "strutils_test.log";

    return K273::Runner::Main(run, config);
}
