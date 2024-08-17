/// ===============================================================================================
/// @file      : combine.hpp                                               |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <cstddef>
#include <cstdint>

#include "_gf8.hpp"

namespace share::codec::helpers
{
template <
	typename Generator,
	typename Vector,
	typename Matrix>
static inline auto combine(
	const Matrix &input,
	uint32_t seed, uint8_t field, uint8_t sparsity,
	Vector &output)
{
	using Value = typename Vector::value_type;
	// combine loop
	auto aux = Vector(output.capacity());
	auto gen = Generator{seed};
	auto factor = Value{0};
	auto counter = size_t{0};
	for (auto &frame : input)
	{
		factor = gen();
		if (factor > sparsity)
			continue;
		factor &= field;
		if (factor == 0)
			continue;
		// copy for calculation process
		aux.assign(frame.begin(), frame.end());
		// calculation process (Y += Xn * Cn)
		gf8::sum(output, gf8::mul(aux, factor));
		// track number of merges
		++counter;
	}
	return counter;
}
} // namespace share::codec::helpers