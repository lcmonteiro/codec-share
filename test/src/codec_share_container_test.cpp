#include <gtest/gtest.h>

#include "container.hpp"


TEST(codec_shared_container, negative_test) {
    auto matrix = codec::container<std::vector<uint8_t>>();

    matrix.push_back(std::vector<uint8_t>(10, 1));

    EXPECT_ANY_THROW(matrix.push_back(std::vector<uint8_t>(9, 1)));
}
