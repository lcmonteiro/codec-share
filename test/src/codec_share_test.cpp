#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "encoder.hpp"
#include "decoder.hpp"

TEST(codec, base) {
    // Properties
    auto VECTOR_SIZE = 10000;
    auto MATRIX_SIZE = 50;

    // First create an instance of an engine
    std::random_device rnd;
    // Specify the engine
    std::mt19937 engine{rnd()};
    // Specify th distribution
    std::uniform_int_distribution<uint8_t> dist{0, 255};
    // Generate data
    std::vector<std::vector<uint8_t>> data_in;
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        std::vector<uint8_t> vec(VECTOR_SIZE);
        std::generate(std::begin(vec), std::end(vec), [&dist, &engine]() {
            return dist(engine);
        });
        data_in.emplace_back(std::move(vec));
    }
    // Encoder
    Codec::Encoder en(data_in);
    // Encode
    auto coded = en.pop(data_in.size());
    // Decoder
    Codec::Decoder de(coded);
    // Decode
    auto data_out = de.pop();
    // Check
    EXPECT_TRUE(data_out == data_in);
}