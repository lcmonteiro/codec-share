/// @file  : token.hpp
/// @author: Luis Monteiro
/// @date  : Created on September 29, 2015, 5:48 PM

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

// Codec Token
namespace codec {
namespace token {
    /// Token Stamp
    ///   is a group of densities
    ///   where a density is a pair of:
    ///   - 1º field size
    ///   - 2º sparsity
    using Stamp = std::vector<std::pair<uint8_t, uint8_t>>;

    /// shared key
    namespace shared {
        using Stamp = std::shared_ptr<const Stamp>;
    }

    /// Type of Tokens
    /// - Sparse
    /// - Stream
    /// - Message
    /// - Full
    enum class Type { SPARSE, STREAM, MESSAGE, FULL };

    /// Default Tokens by type
    /// @param type
    shared::Stamp get(Type type);

    /// Generate Tokens by type
    /// @param type
    shared::Stamp generate(Type type, size_t seed);

} // namespace Token
} // namespace Codec
