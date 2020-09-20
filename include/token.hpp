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
    /// Token Key
    ///   is a group of densities
    ///   where a density is a pair of:
    ///   - 1º field size
    ///   - 2º sparsity
    using Key = std::vector<std::pair<uint8_t, uint8_t>>;
    
    /// shared key
    namespace Shared {
        using Key = std::shared_ptr<const Key>;
    }
    /// unique key
    namespace Unique {
        using Key = std::unique_ptr<const Key>;
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
    Shared::Key Default(Type type);

    /// Generate Tokens by type
    /// @param type
    Unique::Key Generate(Type type);

} // namespace Token
} // namespace Codec
