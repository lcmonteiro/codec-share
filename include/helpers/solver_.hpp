#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <execution>
namespace share::codec::helpers
{
template <typename Vector, typename Func>
void apply(Vector &out, const Vector &in, Func op, size_t offset)
{
	auto p0 = std::next(std::begin(out), offset);
	auto p1 = std::next(std::begin(in), offset);
	auto pe = std::end(out);
	std::transform(p0, pe, p1, p0, op);
}
template <typename Vector, typename Func>
void apply(Vector &out, const Vector &in, Func op)
{
	auto p0 = std::begin(out);
	auto p1 = std::begin(in);
	auto pe = std::end(out);
	std::transform(p0, pe, p1, p0, op);
}
template <typename Vector, typename Type, typename Func>
void apply(Vector &out, Type in, Func op, size_t offset)
{
	auto p0 = std::next(std::begin(out), offset);
	auto pe = std::end(out);
	std::transform(p0, pe, p0, [&op, &in](auto &a) { return op(a, in); });
}
template <typename Vector, typename Type, typename Func>
void apply(Vector &out, Type in, Func op)
{
	auto p0 = std::begin(out);
	auto pe = std::end(out);
	std::transform(p0, pe, p0, [&op, &in](auto &a) { return op(a, in); });
}

template <typename Space, typename MatrixA, typename MatrixB>
inline bool forward_prepare(size_t index, MatrixA &coef, MatrixB &data)
{
	if (!Space::IsZero(coef[index][index]))
	{
		return true;
	}
	for (auto i = index + 1, ii = data.size(); i < ii; ++i)
	{
		if (!Space::IsZero(coef[i][index]))
		{
			std::swap(coef[index], coef[i]);
			std::swap(data[index], data[i]);
			return true;
		}
	}
	return false;
}
template <typename Vector>
inline void sum(Vector &a, Vector &b)
{
	auto *p0 = reinterpret_cast<uint64_t *>(a.data());
	auto *p1 = reinterpret_cast<uint64_t *>(b.data());
	auto *pe = reinterpret_cast<uint64_t *>(a.data() + a.size());
	for (; p0 < pe; ++p0, ++p1)
	{
		*p0 ^= *p1;
	}
}
template <typename Space, typename MatrixA, typename MatrixB>
inline void forward_elimination(size_t index, MatrixA &coef, MatrixB &data)
{
	const auto coef_reference = coef[index][index];
	for (auto i = size_t{index + 1}; i < data.size(); ++i)
	{
		const auto coef_to_reset = coef[i][index];
		if (Space::IsZero(coef_to_reset))
		{
			continue;
		}
		const auto factor =
			Space::Div(coef_reference, coef_to_reset);
		if (Space::IsOne(factor))
		{
			apply(coef[i], coef[index], Space::Sub, index);
			apply(data[i], data[index], Space::Sub);
			continue;
		}

		apply(coef[i], factor, Space::Mul, index);
		apply(coef[i], coef[index], Space::Sub, index);
		apply(data[i], factor, Space::Mul);
		//apply(data[i], data[index], Space::Sub);
		sum(data[i], data[index]);
		/*
		apply_(
			data[i],
			data[index],
			[&factor](auto &a, auto &b) {
				return Space::Sub(
					Space::Mul(a, factor), b);
			});
		*/
	}
}

template <typename Space, typename MatrixA, typename MatrixB>
inline void backward_elimination(size_t index, MatrixA &coef, MatrixB &data)
{
	for (auto i = size_t{0}; i < index; ++i)
	{
		const auto coef_reference = coef[index][index];
		const auto coef_to_reset = coef[i][index];
		if (Space::IsZero(coef_to_reset))
		{
			continue;
		}
		const auto factor =
			Space::Div(coef_to_reset, coef_reference);
		if (Space::IsOne(factor))
		{
			apply(coef[i], coef[index], Space::Sub, index);
			apply(data[i], data[index], Space::Sub);
			continue;
		}
		apply(coef[index], factor, Space::Mul, index);
		apply(data[index], factor, Space::Mul);
		apply(coef[i], coef[index], Space::Sub, index);
		apply(data[i], data[index], Space::Sub);
	}
}

template <typename Space, typename MatrixA, typename MatrixB>
static inline void normalization(size_t index, MatrixA &coef, MatrixB &data)
{
	const auto factor =
		Space::Div(Space::One(), coef[index][index]);
	if (Space::IsOne(factor))
	{
		return;
	}
	apply(coef[index], factor, Space::Mul, index);
	apply(data[index], factor, Space::Mul);
}

template <typename Space, typename MatrixA, typename MatrixB>
inline auto solve(size_t max_size, MatrixA &coef, MatrixB &data)
{
	auto n = size_t{0};
	for (; n < max_size && n < data.size(); ++n)
	{
		if (not forward_prepare<Space>(n, coef, data))
		{
			break;
		}
		forward_elimination<Space>(n, coef, data);
	}
	for (auto i = size_t{1}; i < n; ++i)
	{
		backward_elimination<Space>(i, coef, data);
	}
	for (auto i = size_t{0}; i < n; ++i)
	{
		normalization<Space>(i, coef, data);
	}
	return n;
}
} // namespace share::codec::helpers