
// k273 includes
#include <k273/inplist.h>
#include <k273/exception.h>
#include <k273/logging.h>

using namespace std;
using namespace K273;

class BlaBla : public InplaceListEntry {
  public:
    BlaBla() :
        x(0) {
    }

    int x;
};

#define ASSERT_RAISES(statement)                                        \
    {                                                                   \
        bool good = true;                                               \
        try {                                                           \
            statement;                                                  \
            good = false;                                               \
        } catch (...) {                                                 \
            /* good */                                                  \
        }                                                               \
        if (!good) {                                                    \
            throw Exception("ASSERT_RAISES failed " #statement);        \
        }                                                               \
    }                                                                   \

void test_creation() {

    for (int ii=0; ii<100; ii++) {
        InplaceList l;

        InplaceListEntry a;
        BlaBla b;
        InplaceList c;

        ASSERT (l.empty());
        l.pushBack(&a);
        ASSERT (! l.empty());
        l.pushBack(&b);
        l.pushBack(&c);
        ASSERT (l.size() == 3);
    }
}

void test_insertRemove() {
    InplaceList l;

    // one element
    {
        InplaceListEntry a;

        l.pushBack(&a);
        ASSERT_RAISES (l.pushBack(&a));
        ASSERT_RAISES (l.pushFront(&a));

        l.remove(&a);
        ASSERT_RAISES (l.remove(&a));

        l.pushFront(&a);
        ASSERT_RAISES (l.pushBack(&a));
        ASSERT_RAISES (l.pushFront(&a));

        l.remove(&a);
        ASSERT_RAISES (l.remove(&a));
    }

    // two elements adding pushBack
    {
        InplaceList a;
        InplaceList b;

        // add both
        l.pushBack(&a);
        l.pushBack(&b);

        ASSERT (l.size() == 2);

        ASSERT_RAISES (l.pushBack(&a));
        ASSERT_RAISES (l.pushBack(&b));

        ASSERT (l.size() == 2);

        // remove lifo
        l.remove(&b);
        ASSERT_RAISES (l.remove(&b));

        ASSERT (l.size() == 1);

        l.remove(&a);
        ASSERT_RAISES (l.remove(&a));

        ASSERT (l.size() == 0);

        // add both
        l.pushBack(&a);
        l.pushBack(&b);
        ASSERT_RAISES (l.pushBack(&a));
        ASSERT_RAISES (l.pushBack(&b));

        ASSERT (l.size() == 2);

        // remove fifo
        l.remove(&b);
        ASSERT_RAISES (l.remove(&b));

        ASSERT (l.size() == 1);

        l.remove(&a);
        ASSERT_RAISES (l.remove(&a));

        ASSERT (l.size() == 0);
    }

    // two elements adding pushFront
    {
        InplaceList a;
        InplaceList b;

        // add both
        l.pushFront(&a);
        l.pushFront(&b);

        ASSERT (l.size() == 2);

        ASSERT_RAISES (l.pushFront(&a));
        ASSERT_RAISES (l.pushFront(&b));

        ASSERT (l.size() == 2);

        // remove lifo
        l.remove(&b);
        ASSERT_RAISES (l.remove(&b));

        ASSERT (l.size() == 1);

        l.remove(&a);
        ASSERT_RAISES (l.remove(&a));

        ASSERT (l.size() == 0);

        // add both
        l.pushFront(&a);
        l.pushFront(&b);
        ASSERT_RAISES (l.pushFront(&a));
        ASSERT_RAISES (l.pushFront(&b));

        ASSERT (l.size() == 2);

        // remove fifo
        l.remove(&b);
        ASSERT_RAISES (l.remove(&b));

        ASSERT (l.size() == 1);

        l.remove(&a);
        ASSERT_RAISES (l.remove(&a));

        ASSERT (l.size() == 0);
    }

    // 3 elements
    {
        InplaceList a;
        InplaceList b;
        InplaceList c;

        l.pushFront(&a);
        l.pushFront(&b);
        l.pushFront(&c);
        ASSERT_RAISES (l.pushFront(&a));
        ASSERT_RAISES (l.pushFront(&b));
        ASSERT_RAISES (l.pushFront(&c));

        ASSERT (l.size() == 3);
        l.remove(&a);
        l.remove(&b);
        l.remove(&c);
        ASSERT (l.size() == 0);

        l.pushBack(&a);
        l.pushBack(&b);
        l.pushBack(&c);
        ASSERT_RAISES (l.pushBack(&a));
        ASSERT_RAISES (l.pushBack(&b));
        ASSERT_RAISES (l.pushBack(&c));

        ASSERT (l.size() == 3);
        l.remove(&c);
        l.remove(&b);
        l.remove(&a);
        ASSERT (l.size() == 0);
    }
}

void test_insertBefore() {

    InplaceList l;

    InplaceListEntry a;
    InplaceListEntry b;
    InplaceListEntry c;

    l.pushBack(&a);
    l.insertBefore(&a, &b);
    l.insertBefore(&b, &c);
    l.remove(&a);
    l.insertBefore(&c, &a);

    {
        InplaceListEntry *cur = l.head();
        ASSERT (cur == &a);
        cur = InplaceList::next(cur);
        ASSERT (cur == &c);
        cur = InplaceList::next(cur);
        ASSERT (cur == &b);
        cur = InplaceList::next(cur);
        ASSERT (cur == nullptr);

        ASSERT (l.tail() == &b);
    }

    l.remove(&c);

    {
        InplaceListEntry *cur = l.head();
        ASSERT (cur == &a);
        cur = InplaceList::next(cur);
        ASSERT (cur == &b);
        cur = InplaceList::next(cur);
        ASSERT (cur == nullptr);

        ASSERT (l.tail() == &b);
    }

    l.remove(&b);
    l.remove(&a);
    ASSERT (l.empty());
}

class NumberEntry : public InplaceListEntry {
public:
    NumberEntry(int v) :
        InplaceListEntry(),
        x(v) {
    }

    virtual ~NumberEntry() {
    }

public:
    int x;
};

void test_iteration() {
    NumberEntry a(1);
    NumberEntry b(2);
    NumberEntry c(3);
    InplaceList l;

    l.pushBack(&a);
    l.pushBack(&c);
    l.pushBack(&b);

    for (NumberEntry* e : InplaceRange<NumberEntry>(&l)) {
        std::printf("Here1 %d\n", e->x);
        e->x++;
    }

    for (const NumberEntry* e : InplaceRange<NumberEntry>(&l)) {
        std::printf("Here2 %d\n", e->x);

        //e->x++; <- MUST NOT COMPILE.
    }
}

///////////////////////////////////////////////////////////////////////////////

#define TEST(name)                              \
    printf(" --> test_%s()... ", #name);        \
    test_##name();                              \
    printf("done\n");

void run(vector <string>& args) {
    TEST(creation);
    TEST(insertRemove);
    TEST(insertBefore);
    TEST(iteration);
}

///////////////////////////////////////////////////////////////////////////////

#include <k273/runner.h>

int main(int argc, char** argv) {
    K273::Runner::Config config(argc, argv);
    config.log_filename = "inplist.log";

    return K273::Runner::Main(run, config);
}
