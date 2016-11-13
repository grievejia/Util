#include "Util/Optional.h"

#include "gtest/gtest.h"

#include <complex>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace util;

namespace {

struct caller {
    template <class T>
    caller(T fun) {
        fun();
    }
};

enum State {
    sDefaultConstructed,
    sValueCopyConstructed,
    sValueMoveConstructed,
    sCopyConstructed,
    sMoveConstructed,
    sMoveAssigned,
    sCopyAssigned,
    sValueCopyAssigned,
    sValueMoveAssigned,
    sMovedFrom,
    sValueConstructed
};

struct OracleVal {
    State s;
    int i;
    OracleVal(int i = 0) : s(sValueConstructed), i(i) {}
};

struct Oracle {
    State s;
    OracleVal val;

    Oracle() : s(sDefaultConstructed) {}
    Oracle(const OracleVal& v) : s(sValueCopyConstructed), val(v) {}
    Oracle(OracleVal&& v) : s(sValueMoveConstructed), val(std::move(v)) {
        v.s = sMovedFrom;
    }
    Oracle(const Oracle& o) : s(sCopyConstructed), val(o.val) {}
    Oracle(Oracle&& o) : s(sMoveConstructed), val(std::move(o.val)) {
        o.s = sMovedFrom;
    }

    Oracle& operator=(const OracleVal& v) {
        s = sValueCopyConstructed;
        val = v;
        return *this;
    }
    Oracle& operator=(OracleVal&& v) {
        s = sValueMoveConstructed;
        val = std::move(v);
        v.s = sMovedFrom;
        return *this;
    }
    Oracle& operator=(const Oracle& o) {
        s = sCopyConstructed;
        val = o.val;
        return *this;
    }
    Oracle& operator=(Oracle&& o) {
        s = sMoveConstructed;
        val = std::move(o.val);
        o.s = sMovedFrom;
        return *this;
    }
};

struct Guard {
    std::string val;
    Guard() : val{} {}
    explicit Guard(std::string s, int = 0) : val(s) {}
    Guard(const Guard&) = delete;
    Guard(Guard&&) = delete;
    void operator=(const Guard&) = delete;
    void operator=(Guard&&) = delete;
};

struct ExplicitStr {
    std::string s;
    explicit ExplicitStr(const char* chp) : s(chp){};
};

struct Date {
    int i;
    Date() = delete;
    Date(int i) : i{i} {};
    Date(Date&& d) : i(d.i) { d.i = 0; }
    Date(const Date&) = delete;
    Date& operator=(const Date&) = delete;
    Date& operator=(Date&& d) {
        i = d.i;
        d.i = 0;
        return *this;
    };
};

bool operator==(Oracle const& a, Oracle const& b) {
    return a.val.i == b.val.i;
}
bool operator!=(Oracle const& a, Oracle const& b) {
    return a.val.i != b.val.i;
}

TEST(OptionalTest, disengaged_ctor) {
    optional<int> o1;
    EXPECT_TRUE(!o1);

    optional<int> o2 = nullopt;
    EXPECT_TRUE(!o2);

    optional<int> o3 = o2;
    EXPECT_TRUE(!o3);

    EXPECT_TRUE(o1 == nullopt);
    EXPECT_TRUE(o1 == optional<int>{});
    EXPECT_TRUE(!o1);
    EXPECT_TRUE(bool(o1) == false);

    EXPECT_TRUE(o2 == nullopt);
    EXPECT_TRUE(o2 == optional<int>{});
    EXPECT_TRUE(!o2);
    EXPECT_TRUE(bool(o2) == false);

    EXPECT_TRUE(o3 == nullopt);
    EXPECT_TRUE(o3 == optional<int>{});
    EXPECT_TRUE(!o3);
    EXPECT_TRUE(bool(o3) == false);

    EXPECT_TRUE(o1 == o2);
    EXPECT_TRUE(o2 == o1);
    EXPECT_TRUE(o1 == o3);
    EXPECT_TRUE(o3 == o1);
    EXPECT_TRUE(o2 == o3);
    EXPECT_TRUE(o3 == o2);
};

TEST(OptionalTest, value_ctor) {
    OracleVal v;
    optional<Oracle> oo1(v);
    EXPECT_TRUE(oo1 != nullopt);
    EXPECT_TRUE(oo1 != optional<Oracle>{});
    EXPECT_TRUE(oo1 == optional<Oracle>{v});
    EXPECT_TRUE(!!oo1);
    EXPECT_TRUE(bool(oo1));
    // NA: EXPECT_TRUE(oo1->s == sValueCopyConstructed);
    EXPECT_TRUE(oo1->s == sMoveConstructed);
    EXPECT_TRUE(v.s == sValueConstructed);

    optional<Oracle> oo2(std::move(v));
    EXPECT_TRUE(oo2 != nullopt);
    EXPECT_TRUE(oo2 != optional<Oracle>{});
    EXPECT_TRUE(oo2 == oo1);
    EXPECT_TRUE(!!oo2);
    EXPECT_TRUE(bool(oo2));
    // NA: EXPECT_TRUE(oo2->s == sValueMoveConstructed);
    EXPECT_TRUE(oo2->s == sMoveConstructed);
    EXPECT_TRUE(v.s == sMovedFrom);

    {
        OracleVal v;
        optional<Oracle> oo1{in_place, v};
        EXPECT_TRUE(oo1 != nullopt);
        EXPECT_TRUE(oo1 != optional<Oracle>{});
        EXPECT_TRUE(oo1 == optional<Oracle>{v});
        EXPECT_TRUE(!!oo1);
        EXPECT_TRUE(bool(oo1));
        EXPECT_TRUE(oo1->s == sValueCopyConstructed);
        EXPECT_TRUE(v.s == sValueConstructed);

        optional<Oracle> oo2{in_place, std::move(v)};
        EXPECT_TRUE(oo2 != nullopt);
        EXPECT_TRUE(oo2 != optional<Oracle>{});
        EXPECT_TRUE(oo2 == oo1);
        EXPECT_TRUE(!!oo2);
        EXPECT_TRUE(bool(oo2));
        EXPECT_TRUE(oo2->s == sValueMoveConstructed);
        EXPECT_TRUE(v.s == sMovedFrom);
    }
};

TEST(OptionalTest, assignment) {
    optional<int> oi;
    oi = optional<int>{1};
    EXPECT_TRUE(*oi == 1);

    oi = nullopt;
    EXPECT_TRUE(!oi);

    oi = 2;
    EXPECT_TRUE(*oi == 2);

    oi = {};
    EXPECT_TRUE(!oi);
};

template <class T>
struct MoveAware {
    T val;
    bool moved;
    MoveAware(T val) : val(val), moved(false) {}
    MoveAware(MoveAware const&) = delete;
    MoveAware(MoveAware&& rhs) : val(rhs.val), moved(rhs.moved) {
        rhs.moved = true;
    }
    MoveAware& operator=(MoveAware const&) = delete;
    MoveAware& operator=(MoveAware&& rhs) {
        val = (rhs.val);
        moved = (rhs.moved);
        rhs.moved = true;
        return *this;
    }
};

TEST(OptionalTest, moved_from_state) {
    // first, test mock:
    MoveAware<int> i{1}, j{2};
    EXPECT_TRUE(i.val == 1);
    EXPECT_TRUE(!i.moved);
    EXPECT_TRUE(j.val == 2);
    EXPECT_TRUE(!j.moved);

    MoveAware<int> k = std::move(i);
    EXPECT_TRUE(k.val == 1);
    EXPECT_TRUE(!k.moved);
    EXPECT_TRUE(i.val == 1);
    EXPECT_TRUE(i.moved);

    k = std::move(j);
    EXPECT_TRUE(k.val == 2);
    EXPECT_TRUE(!k.moved);
    EXPECT_TRUE(j.val == 2);
    EXPECT_TRUE(j.moved);

    // now, test optional
    optional<MoveAware<int>> oi{1}, oj{2};
    EXPECT_TRUE(oi);
    EXPECT_TRUE(!oi->moved);
    EXPECT_TRUE(oj);
    EXPECT_TRUE(!oj->moved);

    optional<MoveAware<int>> ok = std::move(oi);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(!ok->moved);
    EXPECT_TRUE(oi);
    EXPECT_TRUE(oi->moved);

    ok = std::move(oj);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(!ok->moved);
    EXPECT_TRUE(oj);
    EXPECT_TRUE(oj->moved);
};

TEST(OptionalTest, copy_move_ctor_optional_int) {
    optional<int> oi;
    optional<int> oj = oi;

    EXPECT_TRUE(!oj);
    EXPECT_TRUE(oj == oi);
    EXPECT_TRUE(oj == nullopt);
    EXPECT_TRUE(!bool(oj));

    oi = 1;
    optional<int> ok = oi;
    EXPECT_TRUE(!!ok);
    EXPECT_TRUE(bool(ok));
    EXPECT_TRUE(ok == oi);
    EXPECT_TRUE(ok != oj);
    EXPECT_TRUE(*ok == 1);

    optional<int> ol = std::move(oi);
    EXPECT_TRUE(!!ol);
    EXPECT_TRUE(bool(ol));
    EXPECT_TRUE(ol == oi);
    EXPECT_TRUE(ol != oj);
    EXPECT_TRUE(*ol == 1);
};

TEST(OptionalTest, optional_optional) {
    optional<optional<int>> oi1 = nullopt;
    EXPECT_TRUE(oi1 == nullopt);
    EXPECT_TRUE(!oi1);

    {
        optional<optional<int>> oi2{in_place};
        EXPECT_TRUE(oi2 != nullopt);
        EXPECT_TRUE(bool(oi2));
        EXPECT_TRUE(*oi2 == nullopt);
        // EXPECT_TRUE(!(*oi2));
        // std::cout << typeid(**oi2).name() << std::endl;
    }

    {
        optional<optional<int>> oi2{in_place, nullopt};
        EXPECT_TRUE(oi2 != nullopt);
        EXPECT_TRUE(bool(oi2));
        EXPECT_TRUE(*oi2 == nullopt);
        EXPECT_TRUE(!*oi2);
    }

    {
        optional<optional<int>> oi2{optional<int>{}};
        EXPECT_TRUE(oi2 != nullopt);
        EXPECT_TRUE(bool(oi2));
        EXPECT_TRUE(*oi2 == nullopt);
        EXPECT_TRUE(!*oi2);
    }

    optional<int> oi;
    auto ooi = make_optional(oi);
    static_assert(std::is_same<optional<optional<int>>, decltype(ooi)>::value,
                  "");
};

TEST(OptionalTest, example_guard) {
    // FAILS: optional<Guard> ogx(Guard("res1"));
    // FAILS: optional<Guard> ogx = "res1";
    // FAILS: optional<Guard> ogx("res1");
    optional<Guard> oga; // Guard is non-copyable (and non-moveable)
    optional<Guard> ogb(in_place,
                        "res1"); // initialzes the contained value with "res1"
    EXPECT_TRUE(bool(ogb));
    EXPECT_TRUE(ogb->val == "res1");

    optional<Guard> ogc(in_place); // default-constructs the contained value
    EXPECT_TRUE(bool(ogc));
    EXPECT_TRUE(ogc->val == "");

    oga.emplace("res1"); // initialzes the contained value with "res1"
    EXPECT_TRUE(bool(oga));
    EXPECT_TRUE(oga->val == "res1");

    oga.emplace(); // destroys the contained value and
                   // default-constructs the new one
    EXPECT_TRUE(bool(oga));
    EXPECT_TRUE(oga->val == "");

    oga = nullopt; // OK: make disengaged the optional Guard
    EXPECT_TRUE(!(oga));
    // FAILS: ogb = {};                          // ERROR: Guard is not Moveable
};

void process() {}
void process(int) {}
void processNil() {}

TEST(OptionalTest, example1) {

    optional<int> oi;           // create disengaged object
    optional<int> oj = nullopt; // alternative syntax
    oi = oj;                    // assign disengaged object
    optional<int> ok = oj;      // ok is disengaged

    if (oi)
        EXPECT_TRUE(false); // 'if oi is engaged...'
    if (!oi)
        EXPECT_TRUE(true); // 'if oi is disengaged...'

    if (oi != nullopt)
        EXPECT_TRUE(false); // 'if oi is engaged...'
    if (oi == nullopt)
        EXPECT_TRUE(true); // 'if oi is disengaged...'

    EXPECT_TRUE(oi == ok); // two disengaged optionals compare equal

    ///////////////////////////////////////////////////////////////////////////
    optional<int> ol{1}; // ol is engaged; its contained value is 1
    ok = 2;              // ok becomes engaged; its contained value is 2
    oj = ol;             // oj becomes engaged; its contained value is 1

    EXPECT_TRUE(oi != ol); // disengaged != engaged
    EXPECT_TRUE(ok != ol); // different contained values
    EXPECT_TRUE(oj == ol); // same contained value
    EXPECT_TRUE(oi < ol);  // disengaged < engaged
    EXPECT_TRUE(ol < ok);  // less by contained value

    /////////////////////////////////////////////////////////////////////////////
    optional<int> om{1};   // om is engaged; its contained value is 1
    optional<int> on = om; // on is engaged; its contained value is 1
    om = 2;                // om is engaged; its contained value is 2
    EXPECT_TRUE(on != om); // on still contains 3. They are not pointers

    /////////////////////////////////////////////////////////////////////////////
    int i = *ol; // i obtains the value contained in ol
    EXPECT_TRUE(i == 1);
    *ol = 9; // the object contained in ol becomes 9
    EXPECT_TRUE(*ol == 9);
    EXPECT_TRUE(ol == make_optional(9));

    ///////////////////////////////////
    int p = 1;
    optional<int> op = p;
    EXPECT_TRUE(*op == 1);
    p = 2;
    EXPECT_TRUE(*op == 1); // value contained in op is separated from p

    ////////////////////////////////
    if (ol)
        process(*ol); // use contained value if present
    else
        process(); // proceed without contained value

    if (!om)
        processNil();
    else
        process(*om);

    /////////////////////////////////////////
    process(ol.value_or(0)); // use 0 if ol is disengaged

    ////////////////////////////////////////////
    ok = nullopt; // if ok was engaged calls T's dtor
    oj = {};      // assigns a temporary disengaged optional
};

TEST(OptionalTest, example_guard_2) {
    const optional<int> c = 4;
    int i = *c; // i becomes 4
    EXPECT_TRUE(i == 4);
    // FAILS: *c = i;                            // ERROR: cannot assign to
    // const int&
};

TEST(OptionalTest, example_ref) {

    int i = 1;
    int j = 2;
    optional<int&> ora;     // disengaged optional reference to int
    optional<int&> orb = i; // contained reference refers to object i

    *orb = 3; // i becomes 3
    // FAILS: ora = j;                           // ERROR: optional refs do not
    // have assignment from T
    // FAILS: ora = {j};                         // ERROR: optional refs do not
    // have copy/move assignment
    // FAILS: ora = orb;                         // ERROR: no copy/move
    // assignment
    ora.emplace(j); // OK: contained reference refers to object j
    ora.emplace(i); // OK: contained reference now refers to object i

    ora = nullopt; // OK: ora becomes disengaged
};

template <typename T>
T getValue(optional<T> newVal = nullopt, optional<T&> storeHere = nullopt) {
    T cached{};

    if (newVal) {
        cached = *newVal;

        if (storeHere) {
            *storeHere = *newVal; // LEGAL: assigning T to T
        }
    }
    return cached;
}

TEST(OptionalTest, example_optional_arg) {
    int iii = 0;
    iii = getValue<int>(iii, iii);
    iii = getValue<int>(iii);
    iii = getValue<int>();

    {

        optional<Guard> grd1{in_place, "res1", 1}; // guard 1 initialized
        optional<Guard> grd2;

        grd2.emplace("res2", 2); // guard 2 initialized
        grd1 = nullopt;          // guard 1 released

    } // guard 2 released (in dtor)
};

std::tuple<Date, Date, Date> getStartMidEnd() {
    return std::tuple<Date, Date, Date>{Date{1}, Date{2}, Date{3}};
}
void run(Date const&, Date const&, Date const&) {}

TEST(OptionalTest, example_date) {

    optional<Date> start, mid,
        end; // Date doesn't have default ctor (no good default date)

    std::tie(start, mid, end) = getStartMidEnd();
    run(*start, *mid, *end);
};

optional<char> readNextChar() {
    return {};
}

void run(optional<std::string>) {}
void run(std::complex<double>) {}

template <class T>
void assign_norebind(optional<T&>& optref, T& obj) {
    if (optref)
        *optref = obj;
    else
        optref.emplace(obj);
}

template <typename T>
void unused(T&&) {}

TEST(OptionalTest, example_conceptual_model) {

    optional<int> oi = 0;
    optional<int> oj = 1;
    optional<int> ok = nullopt;

    oi = 1;
    oj = nullopt;
    ok = 0;

    unused(oi == nullopt);
    unused(oj == 0);
    unused(ok == 1);
};

TEST(OptionalTest, example_rationale) {

    if (optional<char> ch = readNextChar()) {
        unused(ch);
        // ...
    }

    //////////////////////////////////
    optional<int> opt1 = nullopt;
    optional<int> opt2 = {};

    opt1 = nullopt;
    opt2 = {};

    if (opt1 == nullopt) {
    }
    if (!opt2) {
    }
    if (opt2 == optional<int>{}) {
    }

    ////////////////////////////////

    run(nullopt); // pick the second overload
    // FAILS: run({});              // ambiguous

    if (opt1 == nullopt) {
    } // fine
    // FAILS: if (opt2 == {}) {}   // ilegal

    ////////////////////////////////
    EXPECT_TRUE(optional<unsigned>{} < optional<unsigned>{0});
    EXPECT_TRUE(optional<unsigned>{0} < optional<unsigned>{1});
    EXPECT_TRUE(!(optional<unsigned>{} < optional<unsigned>{}));
    EXPECT_TRUE(!(optional<unsigned>{1} < optional<unsigned>{1}));

    EXPECT_TRUE(optional<unsigned>{} != optional<unsigned>{0});
    EXPECT_TRUE(optional<unsigned>{0} != optional<unsigned>{1});
    EXPECT_TRUE(optional<unsigned>{} == optional<unsigned>{});
    EXPECT_TRUE(optional<unsigned>{0} == optional<unsigned>{0});

    /////////////////////////////////
    optional<int> o;
    o = make_optional(1); // copy/move assignment
    o = 1;                // assignment from T
    o.emplace(1);         // emplacement

    ////////////////////////////////////
    int isas = 0, i = 9;
    optional<int&> asas = i;
    assign_norebind(asas, isas);

    /////////////////////////////////////
    ////optional<std::vector<int>> ov2 = {2, 3};
    ////EXPECT_TRUE(bool(ov2));
    ////EXPECT_TRUE((*ov2)[1] == 3);
    ////
    ////////////////////////////////
    ////std::vector<int> v = {1, 2, 4, 8};
    ////optional<std::vector<int>> ov = {1, 2, 4, 8};

    ////EXPECT_TRUE(v == *ov);
    ////
    ////ov = {1, 2, 4, 8};

    ////std::allocator<int> a;
    ////optional<std::vector<int>> ou { in_place, {1, 2, 4, 8}, a };

    ////EXPECT_TRUE(ou == ov);

    //////////////////////////////
    // inconvenient syntax:
    {

        optional<std::vector<int>> ov2{in_place, {2, 3}};

        EXPECT_TRUE(bool(ov2));
        EXPECT_TRUE((*ov2)[1] == 3);

        ////////////////////////////

        std::vector<int> v = {1, 2, 4, 8};
        optional<std::vector<int>> ov{in_place, {1, 2, 4, 8}};

        EXPECT_TRUE(v == *ov);

        ov.emplace({1, 2, 4, 8});
        /*
              std::allocator<int> a;
              optional<std::vector<int>> ou { in_place, {1, 2, 4, 8}, a };
              EXPECT_TRUE(ou == ov);
        */
    }

    /////////////////////////////////
    {
        typedef int T;
        optional<optional<T>> ot{in_place};
        optional<optional<T>> ou{in_place, nullopt};
        optional<optional<T>> ov{optional<T>{}};

        optional<int> oi;
        auto ooi = make_optional(oi);
        static_assert(
            std::is_same<optional<optional<int>>, decltype(ooi)>::value, "");
    }
};

bool fun(std::string, optional<int> oi = nullopt) {
    return bool(oi);
}

TEST(OptionalTest, example_converting_ctor) {

    EXPECT_TRUE(true == fun("dog", 2));
    EXPECT_TRUE(false == fun("dog"));
    EXPECT_TRUE(false == fun("dog", nullopt)); // just to be explicit
};

TEST(OptionalTest, bad_comparison) {
    optional<int> oi, oj;
    int i;
    bool b = (oi == oj);
    b = (oi >= i);
    b = (oi == i);
    unused(b);
};

//// NOT APPLICABLE ANYMORE
////TEST(OptionalTest, perfect_ctor)
////{
////  //optional<std::string> ois = "OS";
////  EXPECT_TRUE(*ois == "OS");
////
////  // FAILS: optional<ExplicitStr> oes = "OS";
////  optional<ExplicitStr> oes{"OS"};
////  EXPECT_TRUE(oes->s == "OS");
////};

TEST(OptionalTest, value_or) {
    optional<int> oi = 1;
    int i = oi.value_or(0);
    EXPECT_TRUE(i == 1);

    oi = nullopt;
    EXPECT_TRUE(oi.value_or(3) == 3);

    optional<std::string> os{"AAA"};
    EXPECT_TRUE(os.value_or("BBB") == "AAA");
    os = {};
    EXPECT_TRUE(os.value_or("BBB") == "BBB");
};

TEST(OptionalTest, mixed_order) {

    optional<int> oN{nullopt};
    optional<int> o0{0};
    optional<int> o1{1};

    EXPECT_TRUE((oN < 0));
    EXPECT_TRUE((oN < 1));
    EXPECT_TRUE(!(o0 < 0));
    EXPECT_TRUE((o0 < 1));
    EXPECT_TRUE(!(o1 < 0));
    EXPECT_TRUE(!(o1 < 1));

    EXPECT_TRUE(!(oN >= 0));
    EXPECT_TRUE(!(oN >= 1));
    EXPECT_TRUE((o0 >= 0));
    EXPECT_TRUE(!(o0 >= 1));
    EXPECT_TRUE((o1 >= 0));
    EXPECT_TRUE((o1 >= 1));

    EXPECT_TRUE(!(oN > 0));
    EXPECT_TRUE(!(oN > 1));
    EXPECT_TRUE(!(o0 > 0));
    EXPECT_TRUE(!(o0 > 1));
    EXPECT_TRUE((o1 > 0));
    EXPECT_TRUE(!(o1 > 1));

    EXPECT_TRUE((oN <= 0));
    EXPECT_TRUE((oN <= 1));
    EXPECT_TRUE((o0 <= 0));
    EXPECT_TRUE((o0 <= 1));
    EXPECT_TRUE(!(o1 <= 0));
    EXPECT_TRUE((o1 <= 1));

    EXPECT_TRUE((0 > oN));
    EXPECT_TRUE((1 > oN));
    EXPECT_TRUE(!(0 > o0));
    EXPECT_TRUE((1 > o0));
    EXPECT_TRUE(!(0 > o1));
    EXPECT_TRUE(!(1 > o1));

    EXPECT_TRUE(!(0 <= oN));
    EXPECT_TRUE(!(1 <= oN));
    EXPECT_TRUE((0 <= o0));
    EXPECT_TRUE(!(1 <= o0));
    EXPECT_TRUE((0 <= o1));
    EXPECT_TRUE((1 <= o1));

    EXPECT_TRUE(!(0 < oN));
    EXPECT_TRUE(!(1 < oN));
    EXPECT_TRUE(!(0 < o0));
    EXPECT_TRUE(!(1 < o0));
    EXPECT_TRUE((0 < o1));
    EXPECT_TRUE(!(1 < o1));

    EXPECT_TRUE((0 >= oN));
    EXPECT_TRUE((1 >= oN));
    EXPECT_TRUE((0 >= o0));
    EXPECT_TRUE((1 >= o0));
    EXPECT_TRUE(!(0 >= o1));
    EXPECT_TRUE((1 >= o1));
};

struct BadRelops {
    int i;
};

constexpr bool operator<(BadRelops a, BadRelops b) {
    return a.i < b.i;
}
constexpr bool operator>(BadRelops a, BadRelops b) {
    return a.i < b.i;
} // intentional error!

TEST(OptionalTest, bad_relops) {

    BadRelops a{1}, b{2};
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a > b);

    optional<BadRelops> oa = a, ob = b;
    EXPECT_TRUE(oa < ob);
    EXPECT_TRUE(!(oa > ob));

    EXPECT_TRUE(oa < b);
    EXPECT_TRUE(oa > b);

    optional<BadRelops &> ra = a, rb = b;
    EXPECT_TRUE(ra < rb);
    EXPECT_TRUE(!(ra > rb));

    EXPECT_TRUE(ra < b);
    EXPECT_TRUE(ra > b);
};

TEST(OptionalTest, mixed_equality) {

    EXPECT_TRUE(make_optional(0) == 0);
    EXPECT_TRUE(make_optional(1) == 1);
    EXPECT_TRUE(make_optional(0) != 1);
    EXPECT_TRUE(make_optional(1) != 0);

    optional<int> oN{nullopt};
    optional<int> o0{0};
    optional<int> o1{1};

    EXPECT_TRUE(o0 == 0);
    EXPECT_TRUE(0 == o0);
    EXPECT_TRUE(o1 == 1);
    EXPECT_TRUE(1 == o1);
    EXPECT_TRUE(o1 != 0);
    EXPECT_TRUE(0 != o1);
    EXPECT_TRUE(o0 != 1);
    EXPECT_TRUE(1 != o0);

    EXPECT_TRUE(1 != oN);
    EXPECT_TRUE(0 != oN);
    EXPECT_TRUE(oN != 1);
    EXPECT_TRUE(oN != 0);
    EXPECT_TRUE(!(1 == oN));
    EXPECT_TRUE(!(0 == oN));
    EXPECT_TRUE(!(oN == 1));
    EXPECT_TRUE(!(oN == 0));

    std::string cat{"cat"}, dog{"dog"};
    optional<std::string> oNil{}, oDog{"dog"}, oCat{"cat"};

    EXPECT_TRUE(oCat == cat);
    EXPECT_TRUE(cat == oCat);
    EXPECT_TRUE(oDog == dog);
    EXPECT_TRUE(dog == oDog);
    EXPECT_TRUE(oDog != cat);
    EXPECT_TRUE(cat != oDog);
    EXPECT_TRUE(oCat != dog);
    EXPECT_TRUE(dog != oCat);

    EXPECT_TRUE(dog != oNil);
    EXPECT_TRUE(cat != oNil);
    EXPECT_TRUE(oNil != dog);
    EXPECT_TRUE(oNil != cat);
    EXPECT_TRUE(!(dog == oNil));
    EXPECT_TRUE(!(cat == oNil));
    EXPECT_TRUE(!(oNil == dog));
    EXPECT_TRUE(!(oNil == cat));
};

TEST(OptionalTest, const_propagation) {

    optional<int> mmi{0};
    static_assert(std::is_same<decltype(*mmi), int&>::value, "WTF");

    const optional<int> cmi{0};
    static_assert(std::is_same<decltype(*cmi), const int&>::value, "WTF");

    optional<const int> mci{0};
    static_assert(std::is_same<decltype(*mci), const int&>::value, "WTF");

    optional<const int> cci{0};
    static_assert(std::is_same<decltype(*cci), const int&>::value, "WTF");
};

static_assert(std::is_base_of<std::logic_error, bad_optional_access>::value,
              "");

TEST(OptionalTest, safe_value) {

    try {
        optional<int> ovN{}, ov1{1};

        int& r1 = ov1.value();
        EXPECT_TRUE(r1 == 1);

        try {
            ovN.value();
            EXPECT_TRUE(false);
        } catch (bad_optional_access const&) {
        }

        { // ref variant
            int i1 = 1;
            optional<int &> orN{}, or1{i1};

            int& r2 = or1.value();
            EXPECT_TRUE(r2 == 1);

            try {
                orN.value();
                EXPECT_TRUE(false);
            } catch (bad_optional_access const&) {
            }
        }
    } catch (...) {
        EXPECT_TRUE(false);
    }
};

TEST(OptionalTest, optional_ref) {

    // FAILS: optional<int&&> orr;
    // FAILS: optional<nullopt_t&> on;
    int i = 8;
    optional<int&> ori;
    EXPECT_TRUE(!ori);
    ori.emplace(i);
    EXPECT_TRUE(bool(ori));
    EXPECT_TRUE(*ori == 8);
    EXPECT_TRUE(&*ori == &i);
    *ori = 9;
    EXPECT_TRUE(i == 9);

    // FAILS: int& ir = ori.value_or(i);
    int ii = ori.value_or(i);
    EXPECT_TRUE(ii == 9);
    ii = 7;
    EXPECT_TRUE(*ori == 9);

    int j = 22;
    auto&& oj = make_optional(std::ref(j));
    *oj = 23;
    EXPECT_TRUE(&*oj == &j);
    EXPECT_TRUE(j == 23);
};

TEST(OptionalTest, optional_ref_const_propagation) {

    int i = 9;
    const optional<int&> mi = i;
    int& r = *mi;
    optional<const int&> ci = i;
    static_assert(std::is_same<decltype(*mi), int&>::value, "WTF");
    static_assert(std::is_same<decltype(*ci), const int&>::value, "WTF");

    unused(r);
};

TEST(OptionalTest, optional_ref_assign) {

    int i = 9;
    optional<int&> ori = i;

    int j = 1;
    ori = optional<int&>{j};
    ori = {j};
    // FAILS: ori = j;

    optional<int&> orx = ori;
    ori = orx;

    optional<int&> orj = j;

    EXPECT_TRUE(ori);
    EXPECT_TRUE(*ori == 1);
    EXPECT_TRUE(ori == orj);
    EXPECT_TRUE(i == 9);

    *ori = 2;
    EXPECT_TRUE(*ori == 2);
    EXPECT_TRUE(ori == 2);
    EXPECT_TRUE(2 == ori);
    EXPECT_TRUE(ori != 3);

    EXPECT_TRUE(ori == orj);
    EXPECT_TRUE(j == 2);
    EXPECT_TRUE(i == 9);

    ori = {};
    EXPECT_TRUE(!ori);
    EXPECT_TRUE(ori != orj);
    EXPECT_TRUE(j == 2);
    EXPECT_TRUE(i == 9);
};

TEST(OptionalTest, optional_ref_swap) {

    int i = 0;
    int j = 1;
    optional<int&> oi = i;
    optional<int&> oj = j;

    EXPECT_TRUE(&*oi == &i);
    EXPECT_TRUE(&*oj == &j);

    swap(oi, oj);
    EXPECT_TRUE(&*oi == &j);
    EXPECT_TRUE(&*oj == &i);
};

TEST(OptionalTest, optional_initialization) {

    using std::string;
    string s = "STR";

    optional<string> os{s};
    optional<string> ot = s;
    optional<string> ou{"STR"};
    optional<string> ov = string{"STR"};
};

TEST(OptionalTest, optional_hashing) {

    using std::string;

    std::hash<int> hi;
    std::hash<optional<int>> hoi;
    std::hash<string> hs;
    std::hash<optional<string>> hos;

    EXPECT_TRUE(hi(0) == hoi(optional<int>{0}));
    EXPECT_TRUE(hi(1) == hoi(optional<int>{1}));
    EXPECT_TRUE(hi(3198) == hoi(optional<int>{3198}));

    EXPECT_TRUE(hs("") == hos(optional<string>{""}));
    EXPECT_TRUE(hs("0") == hos(optional<string>{"0"}));
    EXPECT_TRUE(hs("Qa1#") == hos(optional<string>{"Qa1#"}));

    std::unordered_set<optional<string>> set;
    EXPECT_TRUE(set.find({"Qa1#"}) == set.end());

    set.insert({"0"});
    EXPECT_TRUE(set.find({"Qa1#"}) == set.end());

    set.insert({"Qa1#"});
    EXPECT_TRUE(set.find({"Qa1#"}) != set.end());
};

// optional_ref_emulation
template <class T>
struct generic {
    typedef T type;
};

template <class U>
struct generic<U&> {
    typedef std::reference_wrapper<U> type;
};

template <class T>
using Generic = typename generic<T>::type;

template <class X>
bool generic_fun() {
    optional<Generic<X>> op;
    return bool(op);
}

TEST(OptionalTest, optional_ref_emulation) {

    optional<Generic<int>> oi = 1;
    EXPECT_TRUE(*oi == 1);

    int i = 8;
    int j = 4;
    optional<Generic<int&>> ori{i};
    EXPECT_TRUE(*ori == 8);
    EXPECT_TRUE((void*)&*ori != (void*)&i); // !DIFFERENT THAN optional<T&>

    *ori = j;
    EXPECT_TRUE(*ori == 4);
};

#if OPTIONAL_HAS_THIS_RVALUE_REFS == 1
TEST(OptionalTest, moved_on_value_or) {

    optional<Oracle> oo{in_place};

    EXPECT_TRUE(oo);
    EXPECT_TRUE(oo->s == sDefaultConstructed);

    Oracle o = std::move(oo).value_or(Oracle{OracleVal{}});
    EXPECT_TRUE(oo);
    EXPECT_TRUE(oo->s == sMovedFrom);
    EXPECT_TRUE(o.s == sMoveConstructed);

    optional<MoveAware<int>> om{in_place, 1};
    EXPECT_TRUE(om);
    EXPECT_TRUE(om->moved == false);

    /*MoveAware<int> m =*/std::move(om).value_or(MoveAware<int>{1});
    EXPECT_TRUE(om);
    EXPECT_TRUE(om->moved == true);

#if OPTIONAL_HAS_MOVE_ACCESSORS == 1
    {
        Date d = optional<Date>{in_place, 1}.value();
        EXPECT_TRUE(d.i); // to silence compiler warning

        Date d2 = *optional<Date>{in_place, 1};
        EXPECT_TRUE(d2.i); // to silence compiler warning
    }
#endif
};
#endif

TEST(OptionalTest, optional_ref_hashing) {

    using std::string;

    std::hash<int> hi;
    std::hash<optional<int&>> hoi;
    std::hash<string> hs;
    std::hash<optional<string&>> hos;

    int i0 = 0;
    int i1 = 1;
    EXPECT_TRUE(hi(0) == hoi(optional<int&>{i0}));
    EXPECT_TRUE(hi(1) == hoi(optional<int&>{i1}));

    string s{""};
    string s0{"0"};
    string sCAT{"CAT"};
    EXPECT_TRUE(hs("") == hos(optional<string&>{s}));
    EXPECT_TRUE(hs("0") == hos(optional<string&>{s0}));
    EXPECT_TRUE(hs("CAT") == hos(optional<string&>{sCAT}));

    std::unordered_set<optional<string&>> set;
    EXPECT_TRUE(set.find({sCAT}) == set.end());

    set.insert({s0});
    EXPECT_TRUE(set.find({sCAT}) == set.end());

    set.insert({sCAT});
    EXPECT_TRUE(set.find({sCAT}) != set.end());
};

struct Combined {
    int m = 0;
    int n = 1;

    constexpr Combined() : m{5}, n{6} {}
    constexpr Combined(int m, int n) : m{m}, n{n} {}
};

struct Nasty {
    int m = 0;
    int n = 1;

    constexpr Nasty() : m{5}, n{6} {}
    constexpr Nasty(int m, int n) : m{m}, n{n} {}

    int operator&() { return n; }
    int operator&() const { return n; }
};

TEST(OptionalTest, arrow_operator) {

    optional<Combined> oc1{in_place, 1, 2};
    EXPECT_TRUE(oc1);
    EXPECT_TRUE(oc1->m == 1);
    EXPECT_TRUE(oc1->n == 2);

    optional<Nasty> on{in_place, 1, 2};
    EXPECT_TRUE(on);
    EXPECT_TRUE(on->m == 1);
    EXPECT_TRUE(on->n == 2);
};

TEST(OptionalTest, arrow_wit_optional_ref) {

    Combined c{1, 2};
    optional<Combined&> oc = c;
    EXPECT_TRUE(oc);
    EXPECT_TRUE(oc->m == 1);
    EXPECT_TRUE(oc->n == 2);

    Nasty n{1, 2};
    Nasty m{3, 4};
    Nasty p{5, 6};

    optional<Nasty&> on{n};
    EXPECT_TRUE(on);
    EXPECT_TRUE(on->m == 1);
    EXPECT_TRUE(on->n == 2);

    on = {m};
    EXPECT_TRUE(on);
    EXPECT_TRUE(on->m == 3);
    EXPECT_TRUE(on->n == 4);

    on.emplace(p);
    EXPECT_TRUE(on);
    EXPECT_TRUE(on->m == 5);
    EXPECT_TRUE(on->n == 6);

    optional<Nasty&> om{in_place, n};
    EXPECT_TRUE(om);
    EXPECT_TRUE(om->m == 1);
    EXPECT_TRUE(om->n == 2);
};

TEST(OptionalTest, no_dangling_reference_in_value) {
    // this mostly tests compiler warnings

    optional<int> oi{2};
    unused(oi.value());
    const optional<int> coi{3};
    unused(coi.value());
};

struct CountedObject {
    static int _counter;
    bool _throw;
    CountedObject(bool b) : _throw(b) { ++_counter; }
    CountedObject(CountedObject const& rhs) : _throw(rhs._throw) {
        if (_throw)
            throw int();
    }
    ~CountedObject() { --_counter; }
};

int CountedObject::_counter = 0;

TEST(OptionalTest, exception_safety) {

    try {
        optional<CountedObject> oo(in_place, true); // throw
        optional<CountedObject> o1(oo);
    } catch (...) {
        //
    }
    EXPECT_TRUE(CountedObject::_counter == 0);

    try {
        optional<CountedObject> oo(in_place, true); // throw
        optional<CountedObject> o1(std::move(oo));  // now move
    } catch (...) {
        //
    }
    EXPECT_TRUE(CountedObject::_counter == 0);
};

TEST(OptionalTest, nested_optional) {

    optional<optional<optional<int>>> o1{nullopt};
    EXPECT_TRUE(!o1);

    optional<optional<optional<int>>> o2{in_place, nullopt};
    EXPECT_TRUE(o2);
    EXPECT_TRUE(!*o2);

    optional<optional<optional<int>>> o3(in_place, in_place, nullopt);
    EXPECT_TRUE(o3);
    EXPECT_TRUE(*o3);
    EXPECT_TRUE(!**o3);
}

//// constexpr tests

// these 4 classes have different noexcept signatures in move operations
struct NothrowBoth {
    NothrowBoth(NothrowBoth&&) noexcept(true){};
    void operator=(NothrowBoth&&) noexcept(true){};
};
struct NothrowCtor {
    NothrowCtor(NothrowCtor&&) noexcept(true){};
    void operator=(NothrowCtor&&) noexcept(false){};
};
struct NothrowAssign {
    NothrowAssign(NothrowAssign&&) noexcept(false){};
    void operator=(NothrowAssign&&) noexcept(true){};
};
struct NothrowNone {
    NothrowNone(NothrowNone&&) noexcept(false){};
    void operator=(NothrowNone&&) noexcept(false){};
};

void test_noexcept() {
    {
        optional<NothrowBoth> b1, b2;
        static_assert(noexcept(optional<NothrowBoth>{constexpr_move(b1)}),
                      "bad noexcept!");
        static_assert(noexcept(b1 = constexpr_move(b2)), "bad noexcept!");
    }
    {
        optional<NothrowCtor> c1, c2;
        static_assert(noexcept(optional<NothrowCtor>{constexpr_move(c1)}),
                      "bad noexcept!");
        static_assert(!noexcept(c1 = constexpr_move(c2)), "bad noexcept!");
    }
    {
        optional<NothrowAssign> a1, a2;
        static_assert(!noexcept(optional<NothrowAssign>{constexpr_move(a1)}),
                      "bad noexcept!");
        static_assert(!noexcept(a1 = constexpr_move(a2)), "bad noexcept!");
    }
    {
        optional<NothrowNone> n1, n2;
        static_assert(!noexcept(optional<NothrowNone>{constexpr_move(n1)}),
                      "bad noexcept!");
        static_assert(!noexcept(n1 = constexpr_move(n2)), "bad noexcept!");
    }
}

void constexpr_test_disengaged() {
    constexpr optional<int> g0{};
    constexpr optional<int> g1{nullopt};
    static_assert(!g0, "initialized!");
    static_assert(!g1, "initialized!");

    static_assert(bool(g1) == bool(g0), "ne!");

    static_assert(g1 == g0, "ne!");
    static_assert(!(g1 != g0), "ne!");
    static_assert(g1 >= g0, "ne!");
    static_assert(!(g1 > g0), "ne!");
    static_assert(g1 <= g0, "ne!");
    static_assert(!(g1 < g0), "ne!");

    static_assert(g1 == nullopt, "!");
    static_assert(!(g1 != nullopt), "!");
    static_assert(g1 <= nullopt, "!");
    static_assert(!(g1 < nullopt), "!");
    static_assert(g1 >= nullopt, "!");
    static_assert(!(g1 > nullopt), "!");

    static_assert((nullopt == g0), "!");
    static_assert(!(nullopt != g0), "!");
    static_assert((nullopt >= g0), "!");
    static_assert(!(nullopt > g0), "!");
    static_assert((nullopt <= g0), "!");
    static_assert(!(nullopt < g0), "!");

    static_assert((g1 != optional<int>(1)), "!");
    static_assert(!(g1 == optional<int>(1)), "!");
    static_assert((g1 < optional<int>(1)), "!");
    static_assert((g1 <= optional<int>(1)), "!");
    static_assert(!(g1 > optional<int>(1)), "!");
    static_assert(!(g1 > optional<int>(1)), "!");
}

constexpr optional<int> g0{};
constexpr optional<int> g2{2};
static_assert(g2, "not initialized!");
static_assert(*g2 == 2, "not 2!");
static_assert(g2 == optional<int>(2), "not 2!");
static_assert(g2 != g0, "eq!");

#if OPTIONAL_HAS_MOVE_ACCESSORS == 1
static_assert(*optional<int>{3} == 3, "WTF!");
static_assert(optional<int>{3}.value() == 3, "WTF!");
static_assert(optional<int>{3}.value_or(1) == 3, "WTF!");
static_assert(optional<int>{}.value_or(4) == 4, "WTF!");
#endif

constexpr optional<Combined> gc0{in_place};
static_assert(gc0->n == 6, "WTF!");

// optional refs
int gi = 0;
constexpr optional<int&> gori = gi;
constexpr optional<int&> gorn{};
constexpr int& gri = *gori;
static_assert(gori, "WTF");
static_assert(!gorn, "WTF");
static_assert(gori != nullopt, "WTF");
static_assert(gorn == nullopt, "WTF");
static_assert(&gri == &*gori, "WTF");

constexpr int gci = 1;
constexpr optional<int const&> gorci = gci;
constexpr optional<int const&> gorcn{};

static_assert(gorcn < gorci, "WTF");
static_assert(gorcn <= gorci, "WTF");
static_assert(gorci == gorci, "WTF");
static_assert(*gorci == 1, "WTF");
static_assert(gorci == gci, "WTF");

namespace constexpr_optional_ref_and_arrow {

constexpr Combined c{1, 2};
constexpr optional<Combined const&> oc = c;
static_assert(oc, "WTF!");
static_assert(oc->m == 1, "WTF!");
static_assert(oc->n == 2, "WTF!");
}

#if OPTIONAL_HAS_CONSTEXPR_INIT_LIST

namespace InitList {

struct ConstInitLister {
    template <typename T>
    constexpr ConstInitLister(std::initializer_list<T> il) : len(il.size()) {}
    size_t len;
};

constexpr ConstInitLister CIL{2, 3, 4};
static_assert(CIL.len == 3, "WTF!");

constexpr optional<ConstInitLister> oil{in_place, {4, 5, 6, 7}};
static_assert(oil, "WTF!");
static_assert(oil->len == 4, "WTF!");
}

#endif // OPTIONAL_HAS_CONSTEXPR_INIT_LIST

// end constexpr tests
}
