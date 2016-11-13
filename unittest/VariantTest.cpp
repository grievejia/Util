#include "Util/Variant.h"

#include "gtest/gtest.h"

#include <mutex>

using namespace util;

namespace {

TEST(VariantTest, InitialIsFirstType) {
    variant<int> v;
    EXPECT_TRUE(!v.valueless_by_exception());
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<int>(v), 0);
}

TEST(VariantTest, CanConstructFirstType) {
    variant<int> v(42);
    EXPECT_EQ(v.index(), 0);
}

TEST(VariantTest, GetValueOfFirstType) {
    variant<int> v(42);
    int& i = get<int>(v);
    EXPECT_EQ(i, 42);
}

TEST(VariantTest, CanConstructSecondType) {
    variant<int, std::string> v(std::string{"Hello"});
    EXPECT_EQ(v.index(), 1);
    auto& s = get<std::string>(v);
    EXPECT_EQ(s, "Hello");
}

TEST(VariantTest, CanMoveVariant) {
    variant<int, std::string> v(std::string{"hello"});
    variant<int, std::string> v2(std::move(v));
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(v.index(), -1);
    auto& s = get<std::string>(v2);
    EXPECT_EQ(s, "hello");
}

TEST(VariantTest, CanCopyVariant) {
    variant<int, std::string> v(std::string{"hello"});
    variant<int, std::string> v2(v);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(v.index(), 1);
    auto& s = get<std::string>(v);
    EXPECT_EQ(s, "hello");
    auto& s2 = get<std::string>(v2);
    EXPECT_EQ(s2, "hello");
}

TEST(VariantTest, CanCopyConstVariant) {
    const variant<int, std::string> v(std::string{"hello"});
    variant<int, std::string> v2(v);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(v.index(), 1);
    auto& s = get<std::string>(v);
    EXPECT_EQ(s, "hello");
    auto& s2 = get<std::string>(v2);
    EXPECT_EQ(s2, "hello");
}

TEST(VariantTest, ConstructFromLvalue) {
    std::vector<int> vec(42);
    variant<std::vector<int>> v(vec);
    EXPECT_EQ(vec.size(), 42u);
    EXPECT_EQ(v.index(), 0);
    auto& vec2 = get<std::vector<int>>(v);
    EXPECT_NE(&vec2, &vec);
    EXPECT_EQ(vec2.size(), 42u);
}

TEST(VariantTest, ConstructFromConstLvalue) {
    const std::vector<int> vec(42);
    variant<std::vector<int>> v(vec);
    EXPECT_EQ(vec.size(), 42u);
    EXPECT_EQ(v.index(), 0);
    auto& vec2 = get<std::vector<int>>(v);
    EXPECT_NE(&vec2, &vec);
    EXPECT_EQ(vec2.size(), 42u);
}

TEST(VariantTest, MoveConstructWithMoveOnlyTypes) {
    auto ui = std::make_unique<int>(42);
    variant<std::unique_ptr<int>> v(std::move(ui));
    EXPECT_EQ(v.index(), 0);
    auto& p2 = get<std::unique_ptr<int>>(v);
    EXPECT_TRUE(p2);
    EXPECT_EQ(*p2, 42);

    variant<std::unique_ptr<int>> v2(std::move(v));
    EXPECT_EQ(v.index(), -1);
    EXPECT_EQ(v2.index(), 0);
    auto& p3 = get<std::unique_ptr<int>>(v2);
    EXPECT_TRUE(p3);
    EXPECT_EQ(*p3, 42);
}

struct CopyCounter {
    unsigned move_construct = 0;
    unsigned copy_construct = 0;
    unsigned move_assign = 0;
    unsigned copy_assign = 0;

    CopyCounter() noexcept {}
    CopyCounter(const CopyCounter& rhs) noexcept
        : move_construct(rhs.move_construct),
          copy_construct(rhs.copy_construct + 1), move_assign(rhs.move_assign),
          copy_assign(rhs.copy_assign) {}
    CopyCounter(CopyCounter&& rhs) noexcept
        : move_construct(rhs.move_construct + 1),
          copy_construct(rhs.copy_construct), move_assign(rhs.move_assign),
          copy_assign(rhs.copy_assign) {}
    CopyCounter& operator=(const CopyCounter& rhs) noexcept {
        move_construct = rhs.move_construct;
        copy_construct = rhs.copy_construct;
        move_assign = rhs.move_assign;
        copy_assign = rhs.copy_assign + 1;
        return *this;
    }
    CopyCounter& operator=(CopyCounter&& rhs) noexcept {
        move_construct = rhs.move_construct;
        copy_construct = rhs.copy_construct;
        move_assign = rhs.move_assign + 1;
        copy_assign = rhs.copy_assign;
        return *this;
    }
};

TEST(VariantTest, CopyAssignSameType) {
    CopyCounter cc;
    variant<CopyCounter> v(cc);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);

    variant<CopyCounter> v2(cc);
    v2 = v;
    EXPECT_EQ(v2.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 0u);
}

struct ThrowingConversion {
    template <typename T>
    operator T() const {
        throw 42;
    }
};

template <typename V>
void empty_variant(V& v) {
    try {
        v.template emplace<0>(ThrowingConversion());
    } catch (int) {
    }
    EXPECT_TRUE(v.valueless_by_exception());
}

TEST(VariantTest, CopyAssignToEmpty) {
    CopyCounter cc;
    variant<CopyCounter> v(cc);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);

    variant<CopyCounter> v2;
    empty_variant(v2);
    v2 = v;
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(v2.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 2u);
    EXPECT_EQ(get<CopyCounter>(v2).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 0u);
}

struct InstanceCounter {
    static unsigned instances;

    InstanceCounter() { ++instances; }
    InstanceCounter(InstanceCounter const& rhs) { ++instances; }
    ~InstanceCounter() { --instances; }
};

unsigned InstanceCounter::instances = 0;

TEST(VariantTest, CopyAssignDiffTypesDestroysOld) {
    variant<InstanceCounter, int> v;
    EXPECT_EQ(InstanceCounter::instances, 1u);
    v = variant<InstanceCounter, int>(InstanceCounter());
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    variant<InstanceCounter, int> v2(42);
    v = v2;
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<int>(v2), 42);
    EXPECT_EQ(get<int>(v), 42);
    EXPECT_EQ(InstanceCounter::instances, 0u);
}

TEST(VariantTest, CopyAssignFromEmpty) {
    variant<InstanceCounter, int> v = InstanceCounter();
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    variant<InstanceCounter, int> v2;
    empty_variant(v2);
    v = v2;
    EXPECT_EQ(v.index(), -1);
    EXPECT_EQ(v2.index(), -1);
    EXPECT_EQ(InstanceCounter::instances, 0u);
}

struct CopyError {};

struct ThrowingCopy {
    int data;

    ThrowingCopy() : data(0) {}
    ThrowingCopy(ThrowingCopy const&) { throw CopyError(); }
    ThrowingCopy(ThrowingCopy&&) { throw CopyError(); }
    ThrowingCopy operator=(ThrowingCopy const&) { throw CopyError(); }
};

TEST(VariantTest, ThrowingCopyAssignLeavsTargetUnchanged) {
    variant<std::string, ThrowingCopy> v = std::string("hello");
    EXPECT_EQ(v.index(), 0);
    variant<std::string, ThrowingCopy> v2{in_place<ThrowingCopy>};
    try {
        v = v2;
        // Should be unreachable
        ASSERT_TRUE(false);
    } catch (CopyError&) {
    }
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<0>(v), "hello");
}

TEST(VariantTest, MoveAssignToEmpty) {
    CopyCounter cc;
    variant<CopyCounter> v(cc);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);

    variant<CopyCounter> v2;
    empty_variant(v2);
    v2 = std::move(v);
    EXPECT_EQ(v.index(), -1);
    EXPECT_EQ(v2.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).move_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 0u);
}

TEST(VariantTest, MoveAssignSameType) {
    CopyCounter cc;
    variant<CopyCounter> v(cc);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);

    variant<CopyCounter> v2(std::move(cc));
    v2 = std::move(v);
    EXPECT_EQ(v.index(), -1);
    EXPECT_EQ(v2.index(), 0);
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 1u);
}

TEST(VariantTest, MoveAssignDiffTypesDestroysOld) {
    variant<InstanceCounter, CopyCounter> v;
    empty_variant(v);
    EXPECT_EQ(InstanceCounter::instances, 0u);
    v = variant<InstanceCounter, CopyCounter>(InstanceCounter());
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    variant<InstanceCounter, CopyCounter> v2{CopyCounter()};
    v = std::move(v2);
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(v2.index(), -1);
    EXPECT_EQ(InstanceCounter::instances, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 2u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, MoveAssignFromEmpty) {
    variant<InstanceCounter, int> v = InstanceCounter();
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    variant<InstanceCounter, int> v2;
    empty_variant(v2);
    v = std::move(v2);
    EXPECT_EQ(v.index(), -1);
    EXPECT_EQ(v2.index(), -1);
    EXPECT_EQ(InstanceCounter::instances, 0u);
}

TEST(VariantTest, EmplaceConstructByType) {
    const char* const msg = "hello";
    variant<int, char const*, std::string> v(in_place<std::string>, msg);
    EXPECT_EQ(v.index(), 2);
    EXPECT_EQ(get<2>(v), msg);
}

TEST(VariantTest, EmplaceConstructByIndex) {
    const char* const msg = "hello";
    variant<int, char const*, std::string> v(in_place<2>, msg);
    EXPECT_EQ(v.index(), 2);
    EXPECT_EQ(get<2>(v), msg);
}

TEST(VariantTest, HoldsAlternativeForEmptyVariant) {
    variant<int, double> v;
    empty_variant(v);
    EXPECT_FALSE(holds_alternative<int>(v));
    EXPECT_FALSE(holds_alternative<double>(v));
}

TEST(VariantTest, HoldsAlternativeForNonEmptyVariant) {
    variant<int, double> v(2.3);
    EXPECT_FALSE(holds_alternative<int>(v));
    EXPECT_TRUE(holds_alternative<double>(v));
}

TEST(VariantTest, AssignFromValueToEmpty) {
    CopyCounter cc;
    variant<int, CopyCounter> v;
    v = cc;
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, AssignFromValueToSameType) {
    CopyCounter cc;
    variant<int, CopyCounter> v(cc);
    v = cc;
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, AssignFromValueDiffTypesDestroysOld) {
    variant<InstanceCounter, CopyCounter> v{InstanceCounter()};
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    v = CopyCounter();
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(InstanceCounter::instances, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, EmplaceFromValueToEmpty) {
    const char* const msg = "hello";
    variant<int, char const*, std::string> v;
    v.emplace<std::string>(msg);
    EXPECT_EQ(v.index(), 2);
    EXPECT_EQ(get<2>(v), msg);
}

TEST(VariantTest, EmplaceFromValueToSameType) {
    CopyCounter cc;
    variant<int, CopyCounter> v(cc);
    v.emplace<CopyCounter>();
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, EmplaceFromValueDiffTypesDestroysOld) {
    variant<InstanceCounter, CopyCounter> v{InstanceCounter()};
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    v.emplace<CopyCounter>();
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(InstanceCounter::instances, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, EmplaceByIndexToEmpty) {
    const char* const msg = "hello";
    variant<int, char const*, std::string> v;
    v.emplace<2>(msg);
    EXPECT_EQ(v.index(), 2);
    EXPECT_EQ(get<2>(v), msg);
}

TEST(VariantTest, EmplaceByIndexToSameType) {
    CopyCounter cc;
    variant<int, CopyCounter> v(cc);
    v.emplace<1>();
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, EmplaceByIndexDiffTypesDestroysOld) {
    variant<InstanceCounter, CopyCounter> v{InstanceCounter()};
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(InstanceCounter::instances, 1u);
    v.emplace<1>();
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(InstanceCounter::instances, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, SwapSameType) {
    variant<int, CopyCounter> v{CopyCounter()};
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);

    CopyCounter cc;
    variant<int, CopyCounter> v2{cc};
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 0u);
    v.swap(v2);

    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 1u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_construct, 2u);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 1u);
}

TEST(VariantTest, SwapDiffTypes) {
    variant<int, CopyCounter> v{CopyCounter()};
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 1u);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);

    variant<int, CopyCounter> v2{42};
    v.swap(v2);

    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v2).copy_construct, 0u);
    EXPECT_LE(get<CopyCounter>(v2).move_construct, 3);
    EXPECT_GT(get<CopyCounter>(v2).move_construct, 1);
    EXPECT_EQ(get<CopyCounter>(v2).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v2).move_assign, 0u);

    v.swap(v2);
    EXPECT_EQ(v2.index(), 0);
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<CopyCounter>(v).copy_construct, 0);
    EXPECT_EQ(get<CopyCounter>(v).move_construct, 4);
    EXPECT_EQ(get<CopyCounter>(v).copy_assign, 0u);
    EXPECT_EQ(get<CopyCounter>(v).move_assign, 0u);
}

TEST(VariantTest, AssignEmptyToEmpty) {
    variant<int> v1, v2;
    empty_variant(v1);
    empty_variant(v2);
    v1 = v2;
    EXPECT_EQ(v1.index(), -1);
    EXPECT_EQ(v2.index(), -1);
}

TEST(VariantTest, SwapEmpties) {
    variant<int> v1, v2;
    empty_variant(v1);
    empty_variant(v2);
    v1.swap(v2);
    EXPECT_EQ(v1.index(), -1);
    EXPECT_EQ(v2.index(), -1);
}

struct VisitorIS {
    int& i;
    std::string& s;

    void operator()(int arg) { i = arg; }
    void operator()(std::string const& arg) { s = arg; }
};

TEST(VariantTest, VisitTest) {
    variant<int, std::string> v(42);

    int i = 0;
    std::string s;
    VisitorIS visitor{i, s};
    visit(visitor, v);
    EXPECT_EQ(i, 42);
    i = 0;
    v = std::string("hello");
    visit(visitor, v);
    EXPECT_EQ(s, "hello");
    try {
        variant<int, std::string> v2;
        empty_variant(v2);
        visit(visitor, v2);
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (bad_variant_access) {
    }
}

TEST(VariantTest, ReferenceMembers) {
    int i = 42;
    variant<int&> v(in_place<0>, i);

    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(&get<int&>(v), &i);
    EXPECT_EQ(&get<0>(v), &i);
}

TEST(VariantTest, Equality) {
    variant<int, double, std::string> v(42);
    variant<int, double, std::string> v2(4.2);
    variant<int, double, std::string> v3(std::string("42"));

    EXPECT_EQ(v, v);
    EXPECT_NE(v, v2);
    EXPECT_NE(v, v3);
    EXPECT_EQ(v2, v2);
    EXPECT_EQ(v3, v3);
    variant<int, double, std::string> v4(v);
    EXPECT_EQ(v, v4);
    v4 = std::move(v2);
    EXPECT_NE(v4, v2);
    EXPECT_EQ(v2, v2);
    EXPECT_NE(v, v2);
    v2 = 3;
    EXPECT_NE(v, v2);
}

TEST(VariantTest, LessThan) {
    variant<int, double, std::string> v(42);
    variant<int, double, std::string> v2(4.2);
    variant<int, double, std::string> v3(std::string("42"));

    EXPECT_TRUE(!(v < v));
    EXPECT_TRUE(v >= v);
    EXPECT_TRUE(v < v2);
    EXPECT_TRUE(v < v3);
    EXPECT_TRUE(v2 < v3);
    variant<int, double, std::string> v4(v);
    EXPECT_TRUE(!(v4 < v));
    EXPECT_TRUE(!(v < v4));
    v4 = std::move(v2);
    EXPECT_TRUE(v2 < v4);
    EXPECT_TRUE(v2 < v);
    EXPECT_TRUE(v2 < v3);
    v2 = 99;
    EXPECT_TRUE(v < v2);
    EXPECT_TRUE(v2 < v4);
    EXPECT_TRUE(v2 < v3);
}

TEST(VariantTest, ConstexprVariant) {
    constexpr variant<int> v(42);
    constexpr int i = get<int>(v);
    EXPECT_EQ(i, 42);
    constexpr variant<int> v2(in_place<0>, 42);
    constexpr int i2 = get<int>(v2);
    EXPECT_EQ(i2, 42);
    constexpr variant<int> v3(in_place<int>, 42);
    constexpr int i3 = get<int>(v3);
    EXPECT_EQ(i3, 42);
    constexpr variant<int, double> v4(4.2);
    constexpr int i4 = v4.index();
    EXPECT_EQ(i4, 1);
    constexpr bool b4 = v4.valueless_by_exception();
    EXPECT_FALSE(b4);
    constexpr variant<int, double> v5;
    constexpr int i5 = v5.index();
    EXPECT_EQ(i5, 0);
    constexpr bool b5 = v5.valueless_by_exception();
    EXPECT_FALSE(b5);
}

struct VisitorISD {
    int& i;
    std::string& s;
    double& d;
    int& i2;

    void operator()(int arg, double d_) {
        i = arg;
        d = d_;
    }
    void operator()(std::string const& arg, double d_) {
        s = arg;
        d = d_;
    }
    void operator()(int arg, int i2_) {
        i = arg;
        i2 = i2_;
    }
    void operator()(std::string const& arg, int i2_) {
        s = arg;
        i2 = i2_;
    }
};

TEST(VariantTest, MultiVisitorTest) {
    variant<int, char, std::string> v(42);
    variant<double, int> v2(4.2);

    int i = 0;
    std::string s;
    double d = 0;
    int i2 = 0;
    VisitorISD visitor{i, s, d, i2};
    visit(visitor, v, v2);
    EXPECT_EQ(i, 42);
    EXPECT_EQ(s, "");
    EXPECT_EQ(d, 4.2);
    EXPECT_EQ(i2, 0);
    i = 0;
    d = 0;
    v = std::string("hello");
    EXPECT_EQ(v.index(), 2);
    v2 = 37;
    visit(visitor, v, v2);
    EXPECT_EQ(i, 0);
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(d, 0);
    EXPECT_EQ(i2, 37);

    variant<double, int> v3;
    empty_variant(v3);
    try {
        visit(visitor, v, v3);
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (bad_variant_access) {
    }
}

TEST(VariantTest, DuplicateTypes) {
    variant<int, int> v(42);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<0>(v), 42);

    variant<int, int> v2(in_place<1>, 42);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<1>(v2), 42);
}

struct NonMovable {
    int i;
    NonMovable() : i(42) {}
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

TEST(VariantTest, NonMovableTypes) {
    variant<NonMovable> v{in_place<0>};
    EXPECT_EQ(get<0>(v).i, 42);
    get<0>(v).i = 37;
    v.emplace<NonMovable>();
    EXPECT_EQ(get<0>(v).i, 42);
}

TEST(VariantTest, DirectInitReferenceMember) {
    int i = 42;
    variant<int&> v(i);
    EXPECT_EQ(&get<int&>(v), &i);
}

TEST(VariantTest, RefTypesPreferedForLvalue) {
    int i = 42;
    variant<int, int&> v(i);
    EXPECT_EQ(v.index(), 1);

    variant<int, int&> v2(42);
    EXPECT_EQ(v2.index(), 0);
}

TEST(VariantTest, ConstructWithConversion) {
    variant<int, std::string> v("hello");
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<1>(v), "hello");
}

TEST(VariantTest, AssignWithConversion) {
    variant<int, std::string> v;
    v = "hello";
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<1>(v), "hello");
}

TEST(VariantTest, VisitorWithNonVoidReturn) {
    variant<int> v(42);
    auto res = visit([](auto i) { return i * 2; }, v);
    EXPECT_EQ(res, 84);
}

TEST(VariantTest, MultiVisitorWithNonVoidReturn) {
    variant<int> v(42);
    variant<double> v2(4.2);

    double res = visit([](auto i, auto j) { return i + j; }, v, v2);
    EXPECT_EQ(res, 46.2);
}

typedef variant<std::vector<int>, std::vector<double>> vv;
unsigned foo(vv v) {
    return v.index();
}

TEST(VariantTest, InitWithInitList) {
    variant<std::vector<int>> v{1, 2, 3, 4};
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<0>(v).size(), 4u);

    EXPECT_EQ(foo({1, 2, 3}), 0u);       // OK
    EXPECT_EQ(foo({1.2, 3.4, 5.6}), 1u); // OK

    // variant< char, std::string > q { { 999 } }; // error: can’t deduce T.
    // variant< char, std::string > r = { 999 }; // valid, but overflows.
    // variant< char, std::string > s = { 'a', 'b', 'c' }; // error.
}

struct vector_type;
typedef variant<int, double, std::string, vector_type> JSON;
struct vector_type {
    std::vector<JSON> vec;
    template <typename T>
    vector_type(std::initializer_list<T> list)
        : vec(list.begin(), list.end()) {}
    vector_type(std::initializer_list<JSON> list)
        : vec(list.begin(), list.end()) {}
};

TEST(VariantTest, JsonTest) {
    JSON v1{1};
    JSON v2{4.2};
    JSON v3{"hello"};
    JSON v4{{1, 2, 3}};
    EXPECT_EQ(v4.index(), 3);
    JSON v5{vector_type{1, 2, "hello"}};
}

TEST(VariantTest, NothrowAssignToVariantHoldingTypeWithThrowingMove) {
    variant<ThrowingCopy, int> v{in_place<0>};
    v = 42;
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<1>(v), 42);
}

TEST(VariantTest, MaybeThrowAssignToVariantHoldingTypeWithThrowingMove) {
    variant<ThrowingCopy, std::string> v{in_place<0>};
    v = "hello";
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<1>(v), "hello");
}

TEST(VariantTest, ThrowingAssignFromTypeLeavesVariantUnchanged) {
    variant<ThrowingCopy, std::string> v{"hello"};
    try {
        v = ThrowingCopy();
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (CopyError&) {
    }
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<1>(v), "hello");
}

TEST(VariantTest, CanEmplaceNonmoveableTypeWhenOtherNothrowMovable) {
    variant<std::string, NonMovable> v{"hello"};
    v.emplace<1>();
    EXPECT_EQ(v.index(), 1);
}

struct NonMovableThrower {
    NonMovableThrower(int i) {
        if (i == 42)
            throw CopyError();
    }

    NonMovableThrower(NonMovableThrower&&) = delete;
    NonMovableThrower& operator=(NonMovableThrower&&) = delete;
};

TEST(VariantTest, ThrowingEmplaceFromNonmovableTypeLeavsVariantEmpty) {
    variant<NonMovableThrower, std::string> v{"hello"};
    try {
        v.emplace<NonMovableThrower>(42);
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (CopyError&) {
    }
    EXPECT_EQ(v.index(), -1);
}

TEST(VariantTest, ThrowingEmplaceWhenStoredTypeCanThrowLeavsVariantEmpty) {
    variant<NonMovableThrower, ThrowingCopy> v{in_place<ThrowingCopy>};
    get<1>(v).data = 21;
    try {
        v.emplace<NonMovableThrower>(42);
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (CopyError&) {
    }
    EXPECT_EQ(v.index(), -1);
}

struct MayThrowA {
    int data;
    MayThrowA(int i) : data(i) {}
    MayThrowA(MayThrowA const& other) : data(other.data) {}
};

struct MayThrowB {
    int data;
    MayThrowB(int i) : data(i) {}
    MayThrowB(MayThrowB const& other) : data(other.data) {}
};

TEST(VariantTest, AfterAssignWhichTriggersBackupStorageCanAssignVariant) {
    variant<MayThrowA, MayThrowB> v{MayThrowA(23)};
    v.emplace<MayThrowB>(42);
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(get<1>(v).data, 42);
    variant<MayThrowA, MayThrowB> v2 = v;
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<1>(v2).data, 42);
    v = MayThrowA(23);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<0>(v).data, 23);
    v2 = v;
    EXPECT_EQ(v2.index(), 0);
    EXPECT_EQ(get<0>(v2).data, 23);
    v2 = MayThrowB(19);
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<1>(v2).data, 19);
    v = v2;
    EXPECT_EQ(v2.index(), 1);
    EXPECT_EQ(get<1>(v2).data, 19);
}

TEST(VariantTest, BackupStorageAndLocalBackup) {
    variant<std::string, ThrowingCopy> v{"hello"};
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<0>(v), "hello");
    try {
        v = ThrowingCopy();
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (CopyError&) {
    }
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(get<0>(v), "hello");
}

struct LargeNoExceptMovable {
    char buf[512];

    LargeNoExceptMovable() noexcept {}
    LargeNoExceptMovable(LargeNoExceptMovable&&) noexcept {}
    LargeNoExceptMovable(LargeNoExceptMovable const&&) noexcept {}
    LargeNoExceptMovable& operator=(LargeNoExceptMovable&&) noexcept {
        return *this;
    }
    LargeNoExceptMovable& operator=(LargeNoExceptMovable const&&) noexcept {
        return *this;
    }
};

TEST(VariantTest, LargetNoexceptMovableAndSmallThrowMovable) {
    variant<LargeNoExceptMovable, MayThrowA, MayThrowB> v{
        LargeNoExceptMovable()};
    v = MayThrowB(21);
    v = LargeNoExceptMovable();
    v = MayThrowA(12);
    EXPECT_LT(sizeof(v), 2 * sizeof(LargeNoExceptMovable));
}

struct LargeMayThrowA {
    char dummy[16];
    LargeMayThrowA();
    LargeMayThrowA(LargeMayThrowA const&) {}
};

TEST(VariantTest, IfEmplaceThrowsVariantIsValueless) {
    variant<int> v;
    EXPECT_FALSE(v.valueless_by_exception());
    EXPECT_EQ(v.index(), 0);
    try {
        v.emplace<0>(ThrowingConversion());
        // Should not reach here
        ASSERT_TRUE(false);
    } catch (...) {
    }
    EXPECT_EQ(v.index(), -1);
    EXPECT_TRUE(v.valueless_by_exception());
}

TEST(VariantTest, VariantOfReferences) {
    static int i = 42;
    constexpr variant<int&> vi(i);
    static_assert(&get<0>(vi) == &i,
                  "constexpr variant of reference fails to work");
    constexpr variant<std::string&, int&> vi2(i);
    static_assert(&get<1>(vi2) == &i,
                  "constexpr variant of reference fails to work");
    constexpr variant<const int&> vi3(i);
    static_assert(&get<0>(vi3) == &i,
                  "constexpr variant of const reference fails to work");
}

TEST(VariantTest, GetIf) {
    constexpr variant<int> cvi(42);
    constexpr variant<double, int, char> cvidc(42);
    constexpr variant<double, int, char> cvidc2(4.2);

    static_assert(get_if<0>(cvi) == &get<0>(cvi),
                  "constexpr get_if fails to work");
    static_assert(get_if<int>(cvi) == &get<0>(cvi),
                  "constexpr get_if fails to work");

    static_assert(!get_if<0>(cvidc), "constexpr get_if fails to work");
    static_assert(get_if<1>(cvidc) == &get<1>(cvidc),
                  "constexpr get_if fails to work");
    static_assert(!get_if<2>(cvidc), "constexpr get_if fails to work");
    static_assert(!get_if<double>(cvidc), "constexpr get_if fails to work");
    static_assert(get_if<int>(cvidc) == &get<1>(cvidc),
                  "constexpr get_if fails to work");
    static_assert(!get_if<char>(cvidc), "constexpr get_if fails to work");

    static_assert(get_if<double>(cvidc2) == &get<0>(cvidc2),
                  "constexpr get_if fails to work");
    static_assert(!get_if<int>(cvidc2), "constexpr get_if fails to work");
    static_assert(!get_if<char>(cvidc2), "constexpr get_if fails to work");

    variant<int> vi(42);
    variant<double, int, char> vidc(42);
    variant<double, int, char> vidc2(4.2);

    EXPECT_EQ(get_if<0>(vi), &get<0>(vi));
    EXPECT_EQ(get_if<int>(vi), &get<0>(vi));

    EXPECT_FALSE(get_if<0>(vidc));
    EXPECT_EQ(get_if<1>(vidc), &get<1>(vidc));
    EXPECT_FALSE(get_if<2>(vidc));
    EXPECT_FALSE(get_if<double>(vidc));
    EXPECT_EQ(get_if<int>(vidc), &get<1>(vidc));
    EXPECT_FALSE(get_if<char>(vidc));

    EXPECT_EQ(get_if<double>(vidc2), &get<0>(vidc2));
    EXPECT_FALSE(get_if<int>(vidc2));
    EXPECT_FALSE(get_if<char>(vidc2));
}

TEST(VariantTest, Npos) {
    static_assert(variant_npos == (size_t)-1, "variant npos fails to work");
    static_assert(std::is_same<decltype(variant_npos), const size_t>::value,
                  "variant npos fails to work");
}

TEST(VariantTest, HoldsAlternative) {
    constexpr variant<int> vi(42);
    static_assert(holds_alternative<int>(vi),
                  "constexpr holds_alternative fails to work");
    constexpr variant<int, double> vi2(42);
    static_assert(holds_alternative<int>(vi2),
                  "constexpr holds_alternative fails to work");
    static_assert(!holds_alternative<double>(vi2),
                  "constexpr holds_alternative fails to work");
    constexpr variant<int, double> vi3(4.2);
    static_assert(!holds_alternative<int>(vi3),
                  "constexpr holds_alternative fails to work");
    static_assert(holds_alternative<double>(vi3),
                  "constexpr holds_alternative fails to work");

    const variant<int, double, std::string> vi4(42);
    EXPECT_TRUE(holds_alternative<int>(vi4));
    EXPECT_TRUE(!holds_alternative<double>(vi4));
    EXPECT_TRUE(!holds_alternative<std::string>(vi4));

    variant<int, double, std::string> vi5("hello42");
    EXPECT_TRUE(!holds_alternative<int>(vi5));
    EXPECT_TRUE(!holds_alternative<double>(vi5));
    EXPECT_TRUE(holds_alternative<std::string>(vi5));
}

struct Identity {
    template <typename T>
    constexpr T operator()(T x) {
        return x;
    }
};

struct Sum {
    template <typename T, typename U>
    constexpr auto operator()(T x, U y) {
        return x + y;
    }
};

TEST(VariantTest, ConstexprVisit) {
    constexpr variant<int, double> vi(42);
    constexpr variant<int, double> vi2(21);
    static_assert(visit(Identity(), vi) == 42, "constexpr visit fails to work");
    static_assert(visit(Sum(), vi, vi2) == 63, "constexpr visit fails to work");
}

TEST(VariantTest, VariantWithNoTypes) {
    static_assert(sizeof(variant<>) > 0, "zero-type variant fails to work");
    static_assert(!std::is_default_constructible<variant<>>::value,
                  "zero-type vairant fails to work");
}

TEST(VariantTest, Monostate) {
    static_assert(std::is_trivial<monostate>::value,
                  "constexpr monostate fails to work");
    static_assert(std::is_nothrow_move_constructible<monostate>::value,
                  "constexpr monostate fails to work");
    static_assert(std::is_nothrow_copy_constructible<monostate>::value,
                  "constexpr monostate fails to work");
    static_assert(std::is_nothrow_move_assignable<monostate>::value,
                  "constexpr monostate fails to work");
    static_assert(std::is_nothrow_copy_assignable<monostate>::value,
                  "constexpr monostate fails to work");
    constexpr monostate m1{}, m2{};
    static_assert(m1 == m2, "constexpr monostate fails to work");
    static_assert(!(m1 != m2), "constexpr monostate fails to work");
    static_assert(m1 >= m2, "constexpr monostate fails to work");
    static_assert(m1 <= m2, "constexpr monostate fails to work");
    static_assert(!(m1 < m2), "constexpr monostate fails to work");
    static_assert(!(m1 > m2), "constexpr monostate fails to work");
}

TEST(VariantTest, Hash) {
    variant<int, std::string> vi(42);
    variant<int, std::string> vi2(vi);

    std::hash<variant<int, std::string>> h;
    static_assert(noexcept(h(vi)), "hash fails to work");
    static_assert(std::is_same<decltype(h(vi)), size_t>::value,
                  "hash fails to work");

    EXPECT_EQ(h(vi), h(vi2));

    monostate m{};
    std::hash<monostate> hm;
    static_assert(noexcept(hm(m)), "hash monostate fails to work");
    static_assert(std::is_same<decltype(hm(m)), size_t>::value,
                  "hash monostate fails to work");
}
}
