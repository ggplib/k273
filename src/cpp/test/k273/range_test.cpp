// k273 includes
#include <k273/strutils.h>
#include <k273/exception.h>

// 3rd party
#include <range/v3/all.hpp>

// std includes
#include <vector>
#include <string>
#include <iostream>


void test_one() {
    using namespace ranges;

    std::vector <int> vi{1,2,3,4,5,6,7,8,9,10};
    auto range = vi | view::remove_if([](int i){return i % 2 == 1;});

    std::cout << range << std::endl;
}

void test_two() {
    using namespace ranges;

    auto range = view::iota(1, 11) | view::remove_if([](int i){return i % 2 == 1;});

    std::cout << range << std::endl;
}


#define TEST(name)                                   \
    std::cerr << " --> test_" << #name << "()... ";  \
    test_##name();                                   \
    std::cerr << "done" << std::endl;


void run(std::vector <std::string>& args) {
    TEST(one);
    TEST(two);
}

///////////////////////////////////////////////////////////////////////////////

#include <k273/runner.h>

int main(int argc, char** argv) {
    K273::Runner::Config config(argc, argv);
    config.log_filename = "range_test.log";

    return K273::Runner::Main(run, config);
}
