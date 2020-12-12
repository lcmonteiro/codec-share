
/// @file:   stamp.cpp
/// @author: Luis Monteiro
/// @date:   Created on November 30, 2015, 5:48 PM

#include <random>

#include "token.hpp"

namespace Codec {
namespace Token {
    using Density = Stamp::value_type;

    /// Defaults
    const std::map<Type, std::shared_ptr<const Stamp>> DEFAULT{
      {Type::SPARSE, std::make_shared<const Stamp>(256, Density{31, 127})},
      {Type::FULL, std::make_shared<const Stamp>(256, Density{255, 255})}};

    /// Templates
    const std::map<Type, std::pair<const Density, const Density>> TEMPLATE{

      {Type::STREAM, /***/ {{1, 40}, /***/ {2, 128}}},
      {Type::SPARSE, /***/ {{1, 20}, /***/ {8, 200}}},
      {Type::MESSAGE, /**/ {{3, 20}, /***/ {8, 255}}},
      {Type::FULL, /*****/ {{8, 255}, /**/ {8, 255}}}};

    /// Default Tokens by type
    /// @param type
    Shared::Stamp Default(Type type) { return DEFAULT.at(type); }

    /// Generate Tokens by type
    /// @param type
    /// @param seed
    Shared::Stamp Generate(Type type, uint64_t seed) {

        // field mask
        auto mask = [](uint8_t nbits) -> uint8_t { return (1 << nbits) - 1; };
        // limits
        auto tmp = TEMPLATE.at(type);
        auto min = tmp.first;
        auto max = tmp.second;
        // generator
        auto gen = std::mt19937_64{seed};
        // init stamp
        auto out = Stamp{256};
        // build stamp
        for (auto& v : out) {
            std::uniform_int_distribution<uint8_t> field(min.first, max.first);
            v.first = mask(field(gen));
            std::uniform_int_distribution<uint8_t> sparse(min.second, max.second);
            v.second = sparse(gen);
        }
        // return a unique pointer
        return std::make_shared<const Stamp>(std::move(out));
    }
} // namespace Token
} // namespace Codec