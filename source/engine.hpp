/// @file engine.hpp
/// @author Luis Monteiro
/// @date November 30, 2015, 5:48 PM

#pragma once

#include <utility>
#include <vector>
#include <random>
#include <functional>
#include <cstdint>

#include "stamp.hpp"

namespace Codec {
/// definitions
using Random    = std::random_device;
using Size      = std::uint32_t;
using Frame     = std::vector<std::uint8_t>;
using Container = std::vector<Frame>;
using Generator = std::minstd_rand0;

namespace Engine {
    /// Coded Header Size
    constexpr Size HeaderSize() { return (sizeof(uint32_t)); }

    /// Encode
    /// @param data
    /// @param stamp
    /// @param rand
    /// @param size
    /// @param code
    size_t Encode(
        const Container& data,
        const Stamp& stamp,
        Random& rand,
        Size size,
        Container& code);

    /// Decode
    /// @param data
    /// @param stamp
    /// @param rand
    /// @param size
    /// @param code
    size_t Decode(
        const Container& code,
        const Stamp& stamp,
        Size capacity,
        Container& data,
        Container& coefs,
        Frame& fields);

} // namespace Engine
} // namespace Codec