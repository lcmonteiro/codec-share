
/// @file:   stamp.cpp
/// @author: Luis Monteiro
/// @date:   Created on November 30, 2015, 5:48 PM

#include <random>

#include "token.hpp"

namespace Codec {
namespace Token {
    using Density = Key::value_type;

    /// Defaults
    const std::map<Type, std::shared_ptr<const Key>> DEFAULT{
        {Type::NONE, std::make_shared<const Key>(256, Density{0, 0})},
        {Type::FULL, std::make_shared<const Key>(256, Density{255, 255})}};

    /// Templates
    const std::map<Type, std::pair<const Density, const Density>> TEMPLATE{
        {Type::NONE, /*****/ {{0, 0}, /******/ {0, 0}}},
        {Type::SPARSE, /***/ {{1, 4}, /******/ {255, 255}}},
        {Type::STREAM, /***/ {{1, 8}, /******/ {3, 127}}},
        {Type::MESSAGE, /**/ {{7, 16}, /*****/ {255, 255}}},
        {Type::FULL, /*****/ {{255, 255}, /**/ {255, 255}}}};

    /// Default Tokens by type
    /// @param type
    Shared::Key Default(Type type) {
		return DEFAULT.at(type);
	}

    /// Generate Tokens by type
    /// @param type
    /// @param seed
    Unique::Key Generate(Type type, uint64_t seed) {
        // limits
        auto tmp = TEMPLATE.at(type);
        auto min = tmp.first;
        auto max = tmp.second;
        // generator
        auto gen = std::mt19937_64{seed};
        // init stamp
        auto out = Key{256};
        // build stamp
        for (auto& v : out) {
            v.first = uint8_t(gen());
            v.first |= min.first;
            v.first &= max.first;
            v.second = uint8_t(gen());
            v.second |= min.second;
            v.second &= max.second;
        }
        // return a unique pointer
        return std::make_unique<const Key>(std::move(out));
    }
} // namespace Token
} // namespace Codec