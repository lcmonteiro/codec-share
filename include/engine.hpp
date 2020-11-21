/// @file engine.hpp
/// @author Luis Monteiro
/// @date November 30, 2015, 5:48 PM

#pragma once

#include <cstdint>
#include <functional>
#include <random>
#include <utility>
#include <vector>

#include "container.hpp"
#include "token.hpp"

namespace Codec {
/// definitions
using Random    = std::random_device;
using Generator = std::minstd_rand0;

namespace Engine {
    using Frames = Container::Frames;
    using Frame  = Container::Frame;
    using Stamp  = Token::Stamp;

    /// Coded Header Size
    constexpr size_t HeaderSize() { return (sizeof(uint32_t)); }

    /// Encode
    /// @param data
    /// @param stamp
    /// @param rand
    /// @param size
    /// @return coded data
    Frames Encode(const Stamp& token, const Frames& data, size_t size);

    /// Decode
    /// @param token
    /// @param code
    /// @param size
    /// @param data
    /// @param coefs
    /// @param fields
    size_t
    Decode(const Stamp& token, Frames code, size_t size, Frames& data, Frames& coefs, Frame& fields);

} // namespace Engine
} // namespace Codec