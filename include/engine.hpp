/// @file engine.hpp
/// @author Luis Monteiro
/// @date November 30, 2015, 5:48 PM

#pragma once

#include <utility>
#include <vector>
#include <random>
#include <functional>
#include <cstdint>

#include "token.hpp"
#include "container.hpp"

namespace Codec {
/// definitions
using Random    = std::random_device;
using Size      = std::uint32_t;
using Generator = std::minstd_rand0;

namespace Engine {
    /// Coded Header Size
    constexpr Size HeaderSize() { return (sizeof(uint32_t)); }

    /// Encode
    /// @param data
    /// @param stamp
    /// @param rand
    /// @param size
    /// @return coded data
    Container::Frames
    Encode(const Token::Stamp& token, const Container::Frames& data, Size size);

    /// Decode
    /// @param token
    /// @param code
    /// @param size
    /// @param data
    /// @param coefs
    /// @param fields
    Size Decode(
        const Token::Stamp& token,
        Container::Frames code,
        Size size,
        Container::Frames& data,
        Container::Frames& coefs,
        Container::Frame& fields);

} // namespace Engine
} // namespace Codec