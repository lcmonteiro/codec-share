#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "helpers/solver.hpp"
#include "helpers/gf8.hpp"
#include "helpers/number.hpp"

using namespace share::codec;
using namespace std::chrono;
//using Space = number::Space;
using Space = gf8::Space;
using Vector = std::vector<Space::Type>;
using Matrix = std::vector<Vector>;

auto print = [](const auto &matrix) {
	for (auto &vector : matrix)
	{
		for (auto &value : vector)
			std::cout
				<< std::setw(2)
				<< std::setfill('0')
				<< std::hex
				<< int(value)
				<< " ";
		std::cout << std::endl;
	}
	std::cout << std::endl;
};

auto transpose(const Matrix &in)
{
	auto out = Matrix{};
	auto it = std::max_element(
		std::cbegin(in),
		std::cend(in),
		[](auto &m, auto &a) {
			return m.size() < a.size();
		});
	if (it == std::cend(in))
		return out;
	for (size_t i = 0; i < it->size(); ++i)
	{
		out.emplace_back();
		std::transform(
			std::cbegin(in),
			std::cend(in),
			std::back_inserter(out.back()),
			[&](const auto &a) {
				return i < a.size() ? a[i] : Space::Zero();
			});
	}
	return out;
}
auto identity(const Matrix &in)
{
	auto out = Matrix{};
	auto it = std::max_element(
		std::cbegin(in),
		std::cend(in),
		[](auto &m, auto &a) {
			return m.size() < a.size();
		});
	if (it == std::cend(in))
		return out;
	for (size_t i = 0; i < it->size(); ++i)
	{
		out.emplace_back();
		for (size_t j = 0; j < it->size(); ++j)
		{
			out.back().emplace_back(
				i == j
					? Space::One()
					: Space::Zero());
		}
	}

	return out;
}
auto operator*(const Matrix &a, const Matrix &b)
{
	auto out = Matrix{};
	for (const auto &row_a : a)
	{
		out.emplace_back();
		for (auto row_b : transpose(b))
		{
			out.back().emplace_back(
				std::transform_reduce(
					std::cbegin(row_a),
					std::cend(row_a),
					std::cbegin(row_b),
					Space::Zero(),
					[](const auto &a, const auto &b) {
						return Space::Add(a, b);
					},
					[](const auto &a, const auto &b) {
						return Space::Mul(a, b);
					}));
		}
	}
	return out;
}
auto inverse(Matrix &&in)
{
	auto out = identity(in);
	helpers::solve<Space>(in.size(), in, out);
	return out;
}

// 1 2 3

int test_solver()
{
	auto x = Matrix{
		{1},
		{2},
		{3}};
	auto h = Matrix{
		{1, 2, 3},
		{2, 5, 9},
		{4, 3, 5},
		{6, 7, 9},
		{0, 6, 4},
		{0, 0, 9},
		{8, 0, 9},
	};
	auto b = h * x;
	//b[0][0] = 1;
	print(h);
	print(x);
	print(b);
	print(inverse(Matrix(h)));
	auto h_t = transpose(h);
	auto h_i = inverse(h_t * h);
	auto h_x = h_i * h_t * b;
	print(h_x);
	print(inverse(std::move(h)) * b);

	return 0;
}