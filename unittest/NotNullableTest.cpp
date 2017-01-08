#include "Util/NotNullable.h"

#include "gtest/gtest.h"

#include <unordered_set>

using namespace util;

using std::shared_ptr;
using std::unique_ptr;
using std::unordered_set;

struct pt_base {
    virtual ~pt_base() {}
};
struct pt : pt_base {
    int x;
    int y;
    pt(int x, int y) : x(x), y(y) {}
};
struct pt_other : pt_base {
    int x;
    int y;
    pt_other(int x, int y) : x(x), y(y) {}
};

void take_nn_unique_ptr(nn<unique_ptr<int>>) {}
void take_nn_unique_ptr_constref(const nn<unique_ptr<int>>&) {}
void take_unique_ptr(unique_ptr<int>) {}
void take_unique_ptr_constref(const unique_ptr<int>&) {}
void take_base_ptr(nn<unique_ptr<pt_base>>) {}
void take_nn_raw_ptr(nn<int*>) {}
void take_nn_const_raw_ptr(nn<const int*>) {}

namespace {

TEST(NotNullableTest, Raw) {
    nn<int*> t = nn<int*>(i_promise_i_checked_for_null, new int(7));
    *t = 42;
    nn<pt*> t2 = nn<pt*>(i_promise_i_checked_for_null, new pt(123, 123));
    t2->x = 1;
    delete static_cast<int*>(t);
    delete static_cast<pt*>(t2);
}

TEST(NotNullableTest, UniquePtr) {
    // Construct and operate on a unique_ptr
    nn<unique_ptr<pt>> p1 = nn_make_unique<pt>(pt{2, 2});
    p1->x = 42;
    *p1 = pt{10, 10};
    p1 = nn_make_unique<pt>(pt{1, 1});

    // Move a unique_ptr.
    take_nn_unique_ptr(nn_make_unique<int>(1));
    take_nn_unique_ptr_constref(nn_make_unique<int>(1));
    take_unique_ptr_constref(nn_make_unique<int>(1));
    take_unique_ptr_constref(nn_make_unique<int>(1));
    nn<unique_ptr<int>> i = nn_make_unique<int>(42);
    take_nn_unique_ptr_constref(i);
    take_unique_ptr_constref(i);
    take_nn_unique_ptr(std::move(i));
    i = nn_make_unique<int>(42);
    take_unique_ptr(std::move(i));

    // Check that it still works if const
    const nn<unique_ptr<pt>> c1 = nn_make_unique<pt>(pt{2, 2});
    c1->x = 42;
    *c1 = pt{10, 10};

    // Check conversions to a base class
    nn<unique_ptr<pt_base>> b1(nn_make_unique<pt>(pt{2, 2}));
    b1 = nn_make_unique<pt>(pt{2, 2});
    take_base_ptr(nn_make_unique<pt>(pt{2, 2}));
}

TEST(NotNullableTest, SharedPtr) {
    // Construct and operate on a shared_ptr
    nn<shared_ptr<pt>> p2 = nn_make_shared<pt>(pt{2, 2});

    p2 = nn_make_shared<pt>(pt{3, 3});
    p2->y = 7;
    *p2 = pt{5, 10};
    nn<shared_ptr<pt>> p3 = p2;
    shared_ptr<pt> normal_shared_ptr = p3;

    // Check that it still works if const
    const nn<shared_ptr<pt>> c2 = p2;
    c2->x = 42;
    *c2 = pt{10, 10};
    shared_ptr<pt> m2 = c2;

    // Check conversions to a base class
    nn<shared_ptr<pt_base>> b2(p2);
    b2 = p2;

    // Check nn_shared_ptr cast helpers: static cast to derived class
    nn<shared_ptr<pt_base>> bd1 = nn_make_shared<pt>(3, 4);
    nn<shared_ptr<pt>> ds1 = nn_static_pointer_cast<pt>(bd1);
    assert(ds1->x == 3);
    assert(ds1->y == 4);

    // Check nn_shared_ptr cast helpers: dynamic cast to derived class
    // Not enabled because of -fno-rtti
    // shared_ptr<pt> dd1 = nn_dynamic_pointer_cast<pt>(bd1);
    // assert(dd1);
    // assert(dd1->x == 3);
    // assert(dd1->y == 4);
    // shared_ptr<pt_other> dd_other = nn_dynamic_pointer_cast<pt_other>(bd1);
    // assert(!dd_other);

    // Check nn_shared_ptr cast helpers: const cast
    nn_shared_ptr<pt> ncp1 = nn_make_shared<pt>(3, 4);
    nn_shared_ptr<const pt> cp1 = nn_make_shared<pt>(3, 4);
    nn_shared_ptr<pt> ncp2 = nn_const_pointer_cast<pt>(cp1);
    ncp2->x = 11;
    assert(cp1->x == 11);
    assert(cp1->y == 4);
}

TEST(NotNullableTest, Addr) {
    int i1 = 42;
    take_nn_raw_ptr(nn_addr(i1));
    take_nn_const_raw_ptr(nn_addr(i1));
    const int i2 = 42;
    take_nn_const_raw_ptr(nn_addr(i2));
}

TEST(NotNullableTest, Other) {
    // Check construction of smart pointers from raw pointers
    int* raw1 = new int(7);
    nn<int*> raw2 = nn<int*>(i_promise_i_checked_for_null, new int(7));

    unique_ptr<int> u1(raw1);
    nn<unique_ptr<int>> u2(raw2);

    // Test comparison
    assert(u1 == u1);
    assert(u2 == u2);
    assert(!(u1 == u2));
    assert(!(u1 != u1));
    assert(!(u2 != u2));
    assert(u1 != u2);
    assert(u1 > u2 || u1 < u2);
    assert(u1 >= u2 || u1 <= u2);

    // Test hashing
    unordered_set<nn_shared_ptr<pt>> sset;
    sset.emplace(nn_make_shared<pt>(1, 2));
    unordered_set<nn_unique_ptr<pt>> uset;
    uset.emplace(nn_make_unique<pt>(1, 2));
    unordered_set<nn<pt*>> rset;
    rset.emplace(nn<pt*>(i_promise_i_checked_for_null, new pt(1, 2)));

    nn<shared_ptr<int>> shared = move(u2);
}
}
