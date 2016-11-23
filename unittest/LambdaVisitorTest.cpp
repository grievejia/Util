#include "Util/LambdaVariantVisitor.h"

#include "gtest/gtest.h"

#include <string>

using namespace util;

namespace {
TEST(LambdaVisitorTest, Basic) {
    variant<int, std::string, double> v;
    auto visitor = make_lambda_visitor([](int x) { return 0; },
                                       [](const std::string& x) { return 1; },
                                       [](double x) { return 2; });

    v = 1;
    EXPECT_EQ(visit(visitor, v), 0);

    v = "123";
    EXPECT_EQ(visit(visitor, v), 1);

    v = 0.5;
    EXPECT_EQ(visit(visitor, v), 2);
}
}
