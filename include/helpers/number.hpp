/// ===============================================================================================
/// @file      : gf8.hpp                                                   |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <cstdint>
#include <array>

namespace share::codec::number
{
struct Space
{
	using Type = double;
	static constexpr auto Eq(Type a, Type b)
	{
		const auto delta = a < b ? b - a : a - b;
		const auto epsilon = 0.000001;
		return delta < epsilon;
	}
	static constexpr auto One()
	{
		return Type{1};
	}
	static constexpr auto Zero()
	{
		return Type{0};
	}
	static constexpr auto IsOne(Type a)
	{
		return Eq(One(), a);
	}
	static constexpr auto IsZero(Type a)
	{
		return Eq(Zero(), a);
	}
	static constexpr auto Div(Type a, Type b)
	{
		return a / b;
	}
	static constexpr auto Mul(Type a, Type b)
	{
		return a * b;
	}
	static constexpr auto Sub(Type a, Type b)
	{
		return a - b;
	}
	static constexpr auto Add(Type a, Type b)
	{
		return a + b;
	}
};
} // namespace share::codec::number
