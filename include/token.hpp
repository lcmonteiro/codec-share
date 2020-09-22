/// @file  : token.hpp
/// @author: Luis Monteiro
/// @date  : Created on September 29, 2015, 5:48 PM

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <map>

// Codec Token
namespace Codec {
namespace Token {
    /// Token Stamp
    ///   is a group of densities
    ///   where a density is a pair of:
    ///   - 1º field size
    ///   - 2º sparsity
    using Stamp = std::vector<std::pair<uint8_t, uint8_t>>;
    
    /// shared key
    namespace Shared {
        using Stamp = std::shared_ptr<const Stamp>;
    }
    /// unique key
    namespace Unique {
        using Stamp = std::unique_ptr<const Stamp>;
    }

    /// Type of Tokens
    /// - None
    /// - Sparse
    /// - Stream
    /// - Message
    /// - Full
    enum class Type { NONE, SPARSE, STREAM, MESSAGE, FULL };

    /// Default Tokens by type
    /// @param type
    Shared::Stamp Default(Type type);

    /// Generate Tokens by type
    /// @param type
    Unique::Stamp Generate(Type type);

} // namespace Token
} // namespace Codec
