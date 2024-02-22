/// ===============================================================================================
/// @file      : combine.hpp                                               |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <algorithm>
#include <execution>
#include <cstddef>
#include <cstdint>

#include "gf8_.hpp"
#include "token_.hpp"

namespace share::codec
{
namespace helpers
{
template <
	class Generator,
	class Matrix,
	class Density,
	class Vector>
inline auto make_comb(
	Generator &generator,
	const Matrix &input,
	const Density &density,
	Vector &output)
{
	using gf8 = share::codec::gf8::Space;
	using value_t = typename Vector::value_type;
	// combine loop
	auto cnt = size_t{};
	auto &[field, sparsity] = density;
	for (const auto &frame : input)
	{
		auto factor = value_t(generator());
		if (factor > sparsity)
			continue;
		factor &= field;
		if (factor == 0)
			continue;
		// calculation process (Y += Xn * Cn)
		std::transform(
			std::cbegin(frame),
			std::cend(frame),
			std::cbegin(output),
			std::begin(output),
			[&factor](auto a, auto b) {
				return gf8::Add(gf8::Mul(a, factor), b);
			});
		// track number of merges
		++cnt;
	}
	return cnt;
}
template <
	class Generator,
	class Density,
	class Vector>
inline auto make_coef(
	Generator &generator,
	Density &density,
	Vector &output)
{
	using Value = typename Vector::value_type;
	auto &[field, sparsity] = density;
	for (auto &val : output)
	{
		auto factor = Value(generator());
		if (factor > sparsity)
			continue;
		val = (factor & field);
	}
}
} // namespace helpers
} // namespace share::codec