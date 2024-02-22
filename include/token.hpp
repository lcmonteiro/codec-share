/// ===============================================================================================
/// @file      : token.hpp                                                 |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <random>

// Codec Token
namespace share::codec
{
namespace token
{
/// Token Stamp
///   is a group of densities
///   where a density is a pair of:
///   - 1º field size
///   - 2º sparsity
using Density = std::pair<uint8_t, uint8_t>;
using Stamp = std::vector<Density>;

/// shared key
namespace shared
{
using Stamp = std::shared_ptr<const Stamp>;
}

/// Type of Tokens
/// - Sparse
/// - Stream
/// - Message
/// - Full
enum class Type
{
	SPARSE,
	STREAM,
	MESSAGE,
	FULL
};

/// Defaults Stamps foreach Type
inline const std::map<Type, shared::Stamp> DEFAULT{
	{Type::SPARSE,
	 std::make_shared<const Stamp>(
		 256,
		 Density{31, 127})},
	{Type::FULL,
	 std::make_shared<const Stamp>(
		 256,
		 Density{255, 255})}};

/// Templates Stamps foreach Type
inline const std::map<Type, std::pair<const Density, const Density>> TEMPLATE{
	{Type::STREAM, {{1, 40}, {2, 128}}},
	{Type::SPARSE, {{1, 20}, {8, 200}}},
	{Type::MESSAGE, {{3, 20}, {8, 255}}},
	{Type::FULL, {{8, 255}, {8, 255}}}};

/// Default Tokens by type
/// @param type
inline shared::Stamp get(Type type)
{
	return DEFAULT.at(type);
}

/// Generate Tokens by type
/// @param type
/// @param seed
inline shared::Stamp generate(Type type, uint64_t seed)
{
	// field mask
	auto mask = [](uint8_t nbits) -> uint8_t { return (1 << nbits) - 1; };
	// limits
	auto tmp = TEMPLATE.at(type);
	auto min = tmp.first;
	auto max = tmp.second;
	// generator
	auto gen = std::mt19937_64{seed};
	auto field = std::uniform_int_distribution<uint8_t>{min.first, max.first};
	auto sparse = std::uniform_int_distribution<uint8_t>{min.second, max.second};
	// init stamp
	auto out = Stamp{256};
	// build stamp
	for (auto &[f, s] : out)
	{
		f = mask(field(gen));
		s = sparse(gen);
	}
	// return a unique pointer
	return std::make_shared<const Stamp>(std::move(out));
}
} // namespace token
} // namespace share::codec
