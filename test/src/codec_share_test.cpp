#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "encoder.hpp"
#include "decoder.hpp"

/// CodecEnvironmentParams
struct CodecEnvironmentParams {
    size_t width;
    size_t height;
    size_t redundancy;
    Codec::Token::Type token;
};

/// CodecEnvironment
class CodecEnvironment : public ::testing::TestWithParam<CodecEnvironmentParams> {
  public:
    Codec::Container::Frames Generate(size_t width, size_t height) {
        // First create an instance of an engine
        std::random_device rnd;
        // Specify the engine
        std::mt19937 engine{rnd()};
        // Specify th distribution
        std::uniform_int_distribution<uint8_t> dist{0, 255};
        // Generate data
        std::vector<std::vector<uint8_t>> data;
        for (int i = 0; i < height; ++i) {
            std::vector<uint8_t> vec(width);
            std::generate(
                std::begin(vec), std::end(vec), [&dist, &engine]() { return dist(engine); });
            data.emplace_back(std::move(vec));
        }
        return data;
    }
};

/// Test CodecEnvironment
TEST_P(CodecEnvironment, positive_test) {
    auto params = GetParam();
    // Generate data
    auto input = Generate(params.width, params.height);
    // Generate token
    auto token = Codec::Token::Generate(params.token, 1);
    // Encoder
    Codec::Encoder en(input, token);
    // Encode
    auto coded = en.pop(params.height + params.redundancy);
    // Decoder
    Codec::Decoder de(params.height, coded, token);
    // Decode
    auto output = de.pop();
    // Check
    EXPECT_EQ(output, input);
}
INSTANTIATE_TEST_SUITE_P(
    CodecCommon,
    CodecEnvironment,
    testing::Values(
        CodecEnvironmentParams{1000, 50, 1, Codec::Token::Type::FULL},
        CodecEnvironmentParams{1000, 50, 1, Codec::Token::Type::MESSAGE},
        CodecEnvironmentParams{1000, 50, 5, Codec::Token::Type::STREAM},
        CodecEnvironmentParams{1000, 50, 2, Codec::Token::Type::SPARSE}));