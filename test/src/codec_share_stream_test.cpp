#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <random>

#include "stream.hpp"

TEST(codec_shared_stream, positive_test) {
    using Vector = std::vector<uint8_t>;

    auto is = share::codec::istream<Vector>();
    auto os = share::codec::ostream<Vector>();
    auto in = Vector(10, 1);

    for (auto n = is.set(in, 5, 2); n; --n)
        if (os.push(is.pop()) != 0)
            break;

    EXPECT_EQ(os.get(), in);
}

TEST(codec_shared_stream, positive_test_large) {
    using Vector = std::vector<uint8_t>;

    auto is = share::codec::istream<Vector>();
    auto os = share::codec::ostream<Vector>();
    auto in = Vector(1000);
    std::iota(std::begin(in), std::end(in), uint8_t{0});

    for (auto n = is.set(in, 100, 2); n; --n)
        if (os.push(is.pop()) != 0)
            break;

    EXPECT_EQ(os.get(), in);
}
