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
#include <array>
#include <random>

#include "helpers/traits.hpp"

namespace share::codec
{
namespace token_
{
/// Stamp
///   is a group of densities
///   where a density is a pair of:
///   - 1º field size
///   - 2º sparsity
class Stamp
{
  public:
	struct Density : std::pair<uint8_t, uint8_t>
	{
		using Base = std::pair<uint8_t, uint8_t>;
		Density(uint8_t field, uint8_t sparse)
			: Base{field, sparse} {}
		Density() = default;
		Density(const Density &) = default;
	};
	struct Densities : std::array<Density, 256>
	{
		using Base = std::array<Density, 256>;
		Densities(const Density &init) : Base{}
		{
			fill(init);
		}
		Densities() = default;
		Densities(const Densities &) = default;
	};
	template <class T>
	using if_densities =
		trait::if_convertible<T, Densities>;
	template <class T, if_densities<T> = 0>
	Stamp(T &&init)
		: data_{std::make_shared<Densities>(
			  std::forward<T>(init))} {}
	Stamp(const Stamp &) = default;
	Stamp &operator=(const Stamp &) = default;

	template <class T>
	const auto &operator[](T idx) const
	{
		return (*data_)[uint8_t(idx)];
	}

  private:
	std::shared_ptr<const Densities> data_;
};

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

namespace def
{
/// Default Tokens by type
/// @param type
inline Stamp get(Type type)
{
	static const std::map<Type, Stamp> DEFAULT{
		{Type::SPARSE,
		 Stamp::Densities{Stamp::Density{31, 127}}},
		{Type::FULL,
		 Stamp::Densities{Stamp::Density{255, 255}}}};
	return DEFAULT.at(type);
}

/// Alises
[[maybe_unused]] const auto
	Full = [] { return get(Type::FULL); };
[[maybe_unused]] const auto
	Sparse = [] { return get(Type::SPARSE); };
} // namespace def

namespace gen
{
/// Generate Tokens by type
/// @param type
/// @param seed
inline Stamp create(Type type, uint64_t seed)
{
	/// Templates Stamps foreach Type
	static const std::map<
		Type,
		std::pair<
			const Stamp::Density,
			const Stamp::Density>>
		TEMPLATE{
			{Type::STREAM, {{1, 40}, {2, 128}}},
			{Type::SPARSE, {{1, 20}, {8, 200}}},
			{Type::MESSAGE, {{3, 20}, {8, 255}}},
			{Type::FULL, {{8, 255}, {8, 255}}}};
	// field mask
	static const auto mask =
		[](uint8_t nbits) -> uint8_t {
		return (1 << nbits) - 1;
	};
	// limits
	auto tmp = TEMPLATE.at(type);
	auto min = tmp.first;
	auto max = tmp.second;
	// generator
	auto gen = std::mt19937_64{seed};
	auto field = std::uniform_int_distribution<uint8_t>{min.first, max.first};
	auto sparse = std::uniform_int_distribution<uint8_t>{min.second, max.second};
	// init stamp
	auto out = Stamp::Densities{};
	// build stamp
	for (auto &[f, s] : out)
	{
		f = mask(field(gen));
		s = sparse(gen);
	}
	return Stamp{std::move(out)};
}
/// Alises
[[maybe_unused]] const auto
	Full = [](auto seed) {
		return create(Type::FULL, seed);
	};
[[maybe_unused]] const auto
	Message = [](auto seed) {
		return create(Type::MESSAGE, seed);
	};
} // namespace gen
} // namespace token_
} // namespace share::codec
