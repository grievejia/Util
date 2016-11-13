#include "Util/StopWatch.h"

#include "gtest/gtest.h"

#include <thread>

using namespace util;

namespace {
TEST(StopWatchTest, Basic) {
    StopWatch<std::chrono::milliseconds> watch;
    auto sleepTime = std::chrono::milliseconds(200);
    std::this_thread::sleep_for(sleepTime);
    auto elapsed = watch.elapsed();
    EXPECT_GE(elapsed, sleepTime);
}

TEST(StopWatchTest, Coarse) {
    CoarseStopWatch<std::chrono::milliseconds> watch;
    auto sleepTime = std::chrono::milliseconds(200);
    std::this_thread::sleep_for(sleepTime);
    auto elapsed = watch.elapsed();
    EXPECT_GE(elapsed, sleepTime);
}
}
