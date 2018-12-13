
// k273 includes
#include <k273/strutils.h>
#include <k273/exception.h>

// 3rd party
#include <catch.hpp>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <range/v3/all.hpp>

// std includes
#include <vector>
#include <string>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>


TEST_CASE("listdir() basic - /tmp", "[listdir_basic_tmp_dir]") {
    const auto files = K273::listdir("/tmp");
    REQUIRE(files.size() > 0);
}


TEST_CASE("listdir() basic - /proc", "[listdir_basic_proc]") {
    const pid_t pid = ::getpid();
    const std::string path = K273::fmtString("/proc/%d/fd", pid);

    auto files = K273::listdir(path);

    REQUIRE(files.at(0) == ".");
    REQUIRE(files.at(1) == "..");

    // drop this first 2 elements
    auto files_range = files | ranges::view::drop(2);
    int i = 0;
    for (auto f : files_range) {
        REQUIRE(f == K273::fmtString("%d", i++));
    }

    // ., .., 0, 1, 2
    REQUIRE(files.size() >= 5);
}


TEST_CASE("listdir() no access", "[listdir_no_access]") {
    REQUIRE_THROWS_AS(K273::listdir("/root"), K273::SysException);
}


TEST_CASE("listdir() does not exists", "[listdir_nonexisting_dir]") {
    REQUIRE_THROWS_AS(K273::listdir("/asjfasdf"), K273::SysException);
}


// fmt library tests

TEST_CASE("fmt lib tests", "[fmt_me]") {
    std::string message = fmt::format("The answer is {}", 42);
    REQUIRE(message == "The answer is 42");
}

TEST_CASE("fmt lib tests2", "[fmt_me2]") {
    long x1 = 42;
    long long x2 = 42;
    unsigned long x3 = 42;
    size_t x4 = 42;
    std::string message1 = fmt::sprintf("The answer is %d", x1);
    std::string message2 = fmt::sprintf("The answer is %d", x2);
    std::string message3 = fmt::sprintf("The answer is %d", x3);
    std::string message4 = fmt::sprintf("The answer is %d", x4);

    REQUIRE(message1 == "The answer is 42");
    REQUIRE(message2 == "The answer is 42");
    REQUIRE(message3 == "The answer is 42");
    REQUIRE(message4 == "The answer is 42");

    std::string message5 = fmt::sprintf("The answer is %s", x1);
    REQUIRE(message5 == "The answer is 42");

    std::string message6 = fmt::sprintf("The question is '%s'", message5);
    REQUIRE(message6 == "The question is 'The answer is 42'");
}
