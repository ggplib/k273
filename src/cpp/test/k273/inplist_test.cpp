// k273 includes
#include <k273/inplist.h>
#include <k273/exception.h>
#include <k273/logging.h>

// 3rd party
#include <catch.hpp>
#include <fmt/printf.h>
#include <range/v3/all.hpp>

// std includes
#include <string>
#include <vector>

using namespace K273;

///////////////////////////////////////////////////////////////////////////////

TEST_CASE("basic inplace list test", "[inplace]") {

    using IntList = InplaceList <int>;
    using IntListNode = IntList::Node;

    auto b = new IntListNode(0);
    int& x = b->get();
    x++;
    REQUIRE(b->get() == 1);

    IntList l;
    l.pushBack(b);
    REQUIRE(l.size() == 1);
    l.pushFront(l.createNode(2));
    l.insertBefore(b, l.createNode(l.size() + 1));

    REQUIRE(l.size() == 3);

    REQUIRE(l.head()->get() == 2);

    l.insertAfter(b, l.createNode(l.size() + 1));
    REQUIRE(l.tail()->get() == 4);

    l.insertBefore(b, l.createNode(l.size() + 1));
    REQUIRE(l.size() == 5);

    IntList* ptr_l = b->parent();
    ASSERT(ptr_l == &l);

    int accumulate = 0;
    for (auto n : l) {
        accumulate += n->get();
    }

    REQUIRE(accumulate == 15);

    accumulate = 0;
    for (auto n : l.asData()) {
        accumulate += n;
        fmt::printf(" %d ", n);
    }

    REQUIRE(accumulate == 15);

    for (auto n : l) {
        l.remove(n);
        break;
    }

    REQUIRE(l.size() == 4);
    REQUIRE(l.head()->get() == 3);
}


TEST_CASE("inplace insert remove", "[inplace]") {

    using IntList = InplaceList<int, false>;

    IntList l;

    SECTION("empty") {
        REQUIRE(l.size() == 0);
        l.clear();
        REQUIRE(l.size() == 0);
    }

    SECTION("one element") {
        REQUIRE(l.size() == 0);

        IntList::Node a;

        l.pushBack(&a);
        REQUIRE_THROWS_AS(l.pushBack(&a), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushFront(&a), K273::Assertion);

        l.remove(&a);
        REQUIRE_THROWS_AS(l.remove(&a), K273::Assertion);

        l.pushFront(&a);
        REQUIRE_THROWS_AS(l.pushBack(&a), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushFront(&a), K273::Assertion);

        l.remove(&a);
        REQUIRE_THROWS_AS(l.remove(&a), K273::Assertion);

        l.clear();
        REQUIRE(l.size() == 0);
    }

    SECTION("two elements adding pushBack") {
        REQUIRE(l.size() == 0);

        IntList::Node a;
        IntList::Node b;

        // add both
        l.pushBack(&a);
        l.pushBack(&b);

        REQUIRE(l.size() == 2);

        REQUIRE_THROWS_AS(l.pushBack(&a), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushBack(&b), K273::Assertion);

        REQUIRE(l.size() == 2);

        // remove lifo
        l.remove(&b);
        REQUIRE_THROWS_AS(l.remove(&b), K273::Assertion);

        REQUIRE(l.size() == 1);

        l.remove(&a);
        REQUIRE_THROWS_AS(l.remove(&a), K273::Assertion);

        REQUIRE(l.size() == 0);

        // add both
        l.pushBack(&a);
        l.pushBack(&b);
        REQUIRE_THROWS_AS(l.pushBack(&a), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushBack(&b), K273::Assertion);

        REQUIRE(l.size() == 2);

        // remove fifo
        l.remove(&b);
        REQUIRE_THROWS_AS(l.remove(&b), K273::Assertion);

        REQUIRE(l.size() == 1);

        l.remove(&a);
        REQUIRE_THROWS_AS(l.remove(&a), K273::Assertion);

        REQUIRE(l.size() == 0);
    }

    SECTION("3 elements") {
        REQUIRE(l.size() == 0);

        IntList::Node a;
        IntList::Node b;
        IntList::Node c;

        l.pushFront(&a);
        l.pushFront(&b);
        l.pushFront(&c);
        REQUIRE_THROWS_AS(l.pushFront(&a), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushFront(&b), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushFront(&c), K273::Assertion);

        REQUIRE(l.size() == 3);
        l.remove(&a);
        l.remove(&b);
        l.remove(&c);

        REQUIRE(l.size() == 0);

        l.pushBack(&a);
        l.pushBack(&b);
        l.pushBack(&c);

        REQUIRE_THROWS_AS(l.pushBack(&a), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushBack(&b), K273::Assertion);
        REQUIRE_THROWS_AS(l.pushBack(&c), K273::Assertion);

        REQUIRE(l.size() == 3);
        l.remove(&c);
        l.remove(&b);
        l.remove(&a);

        REQUIRE(l.size() == 0);
    }
}

TEST_CASE("inplace insert before", "[inplace]") {
    using IntList = InplaceList<int, false>;

    IntList l;

    IntList::Node a;
    IntList::Node b;
    IntList::Node c;

    l.pushBack(&a);
    l.insertBefore(&a, &b);
    l.insertBefore(&b, &c);
    l.remove(&a);
    l.insertBefore(&c, &a);


    SECTION("iterate manually over") {
        IntList::Node* cur = l.head();
        REQUIRE(cur == &a);
        cur = cur->next();

        REQUIRE(cur == &c);
        cur = cur->next();

        REQUIRE(cur == &b);
        cur = cur->next();

        REQUIRE(cur == nullptr);

        REQUIRE(l.tail() == &b);
    }

    l.remove(&c);

    SECTION("iterate manually over after remove") {
        IntList::Node* cur = l.head();
        REQUIRE(cur == &a);
        cur = cur->next();

        REQUIRE(cur == &b);

        cur = cur->next();
        REQUIRE(cur == nullptr);

        REQUIRE(l.tail() == &b);
    }

    l.remove(&b);
    l.remove(&a);
    REQUIRE(l.empty());
}

struct NumberEntry {
    NumberEntry(int v) :
        x(v) {
    }

    int x;
};

TEST_CASE("inplace simple iteration", "[inplace]") {
    InplaceList<NumberEntry> l;

    l.pushBack(l.createNode(1));
    l.pushBack(l.createNode(2));
    l.pushBack(l.createNode(3));

    int total = 0;
    for (auto& e : l.asData()) {
        fmt::printf("Here1 %d\n", e.x);
        e.x++;
        total += e.x;
    }

    REQUIRE(total == 9);

   for (const auto n : l) {
        fmt::printf("Here2 %d\n", n->get().x);
        total -= n->get().x;
    }

    REQUIRE(total == 0);
}

struct BlaBla {
    BlaBla(int i) :
        x(i),
        y(i) {
    }

    int x;
    int y;
};

template <typename L>
class ListRange : public ranges::view_facade<ListRange<L>> {
    friend ranges::range_access;
    using Node = typename L::Node;
    using DataType = typename L::DataType;

    Node* node = nullptr;
    const DataType& read() const {
        return this->node->get();
    }

    bool equal(ranges::default_sentinel) const {
        return this->node == nullptr;
    }

    void next() {
        this->node = this->node->next();
    }

public:
    ListRange() = default;

    explicit ListRange(Node* node) :
        node(node) {
    }
};

TEST_CASE("complicated inplace list test", "[inplace]") {

    using BlaList = InplaceList<BlaBla>;

    using namespace ranges;

    for (int ii=0; ii<100; ii++) {
        BlaList l;

        auto a = l.createNode(ii);
        auto b = l.createNode(0);
        auto c = l.createNode(42);

        REQUIRE(l.empty());
        l.pushBack(a);

        REQUIRE(!l.empty());
        l.pushBack(b);
        l.pushBack(c);
        REQUIRE(l.size() == 3);

        int total = 0;
        for (auto o : l.asData()) {
            total += o.x;
        }

        fmt::printf(" total = %d\n", total);

        auto iter_l = ListRange<BlaList>(l.head());
        auto fn = [](auto& n) {
            return n.x > 42;
        };

        auto rr = iter_l | view::remove_if(fn);

        for (auto& zz : rr) {
            fmt::printf(" %d ", zz.x);
        }

        fmt::printf("\n");
    }
}

struct BlaX12 {
    BlaX12(int x, int y) :
        x(x),
        y(y) {
    }

    int x;
    int y;
};

TEST_CASE("inplace emplace", "[inplace]") {

    using IntList = InplaceList <int>;
    using IntListNode = IntList::Node;

    using BlaList = InplaceList<BlaX12>;

    IntList l;
    l.pushBack(new IntListNode(1));
    l.emplaceBack(42);
    REQUIRE(!l.empty());
    REQUIRE(l.size() == 2);

    BlaList l2;

    l2.pushBack(l2.createNode(42, 24));
    l2.emplaceBack(75, 23);
    REQUIRE(!l2.empty());
    REQUIRE(l2.size() == 2);
}
