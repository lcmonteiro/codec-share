#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "container.hpp"

TEST(codec_shared_container, positive_test) {
    auto matrix = codec::container<std::vector<uint8_t>>();
    auto vector = std::vector<uint8_t>(10, 1);

    std::fill_n(std::back_inserter(matrix), 10, vector);

    std::cout << matrix << std::endl;


    // EXPECT_EQ(os.get(), in);
}