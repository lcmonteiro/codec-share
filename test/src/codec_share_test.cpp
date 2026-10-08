#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "decoder.hpp"
#include "encoder.hpp"

/// CodecEnvironmentParams
struct CodecEnvironmentParams {
    size_t width;
    size_t height;
    size_t redundancy;
    share::codec::token::Type token;
};

/// CodecEnvironment
class CodecEnvironment : public ::testing::TestWithParam<CodecEnvironmentParams> {
  public:
    auto generate(size_t width, size_t height) {
        // First create an instance of an engine
        std::random_device rnd;
        // Specify the engine
        std::mt19937 engine{rnd()};
        // Specify th distribution
        std::uniform_int_distribution<uint8_t> dist{0, 255};
        // Generate data
        std::vector<std::vector<uint8_t>> data;
        for (auto i = size_t{}; i < height; ++i) {
            std::vector<uint8_t> vec(width);
            std::generate(std::begin(vec), std::end(vec), [&dist, &engine]() {
                return dist(engine);
            });
            data.emplace_back(std::move(vec));
        }
        return data;
    }
};

/// Test CodecEnvironment
TEST_P(CodecEnvironment, positive_test) {
    auto params  = GetParam();
    auto input   = generate(params.width, params.height);
    auto token   = share::codec::token::generate(params.token, 1);
    auto encoder = share::codec::encoder<std::vector<uint8_t>>(input, token);
    auto coded   = encoder.pop(params.height + params.redundancy);
    auto decoder = share::codec::decoder<std::vector<uint8_t>>(params.height, coded, token);
    auto output  = decoder.pop();

    EXPECT_EQ(output, input);
}
/// Test merges, every coded frame combines all data frames
TEST_P(CodecEnvironment, merges_test) {
    auto params  = GetParam();
    auto input   = generate(1000, 8);
    auto token   = share::codec::token::generate(params.token, 1);
    auto encoder = share::codec::encoder<std::vector<uint8_t>>(input, token);
    auto coded   = encoder.pop(8, 8);
    for (auto& frame : coded)
        for (auto& vec : input)
            EXPECT_FALSE(std::equal(std::begin(vec), std::end(vec), std::begin(frame)));
    auto decoder = share::codec::decoder<std::vector<uint8_t>>(8, coded, token);
    if (decoder.full())
        EXPECT_EQ(decoder.pop(), share::codec::container<std::vector<uint8_t>>(input));
}
INSTANTIATE_TEST_SUITE_P(
  CodecCommon,
  CodecEnvironment,
  testing::Values(
    CodecEnvironmentParams{1000000, 50, 1, share::codec::token::Type::FULL},
    CodecEnvironmentParams{1000000, 50, 1, share::codec::token::Type::MESSAGE},
    CodecEnvironmentParams{1000000, 50, 5, share::codec::token::Type::STREAM},
    CodecEnvironmentParams{1000000, 50, 2, share::codec::token::Type::SPARSE}));
