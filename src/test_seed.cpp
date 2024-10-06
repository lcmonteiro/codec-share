#include <algorithm>
#include <random>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <map>

#include "decoder.hpp"
#include "encoder.hpp"
#include "container.hpp"

static auto generate_input(size_t height)
{
	std::random_device rnd;
	std::mt19937 engine{rnd()};
	std::uniform_int_distribution<uint8_t> dist{0, 255};
	std::vector<std::vector<uint8_t>> data;
	for (auto i = size_t{}; i < height; ++i)
	{
		data.emplace_back(1, i);
	}
	return data;
}
template <class C, class F>
static auto check_combinations(C &code, C &next, F call)
{
	
	for (auto i = size_t{1}; i <= next.height(); ++i)
	{
		auto carrier = code.pop_back(i);
		code.push_front(next.pop_back(i));
		
		if (not call(code))
			return false;
		const auto height = code.height() - i;
		for (auto j = size_t{0}; j < height; ++j)
		{
			code.push_front(carrier.pop_back(1));
			carrier.push_front(code.pop_back(1));
			if (not call(code))
				return false;
		}
		code.push_front(carrier.pop_back(i));
		next.push_front(code.pop_back(i));
	}
	return true;
}

static auto print_matrix = [](const auto &matrix) {
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

static auto print_table = [](const auto &table) {
	for (auto &[count, seed] : table)
	{
		std::cout << "seed= "<< seed << " ";
		std::cout << "count= "<< count << std::endl;
	}
	std::cout << std::endl;
};


using namespace share::codec;
using namespace std::chrono;
using Frame = std::vector<uint8_t>;

int test_seed(
	int beg_seed,
	int end_seed,
	int height = 40,
	int redundancy = 0)
{
	auto input = generate_input(height);
	auto table = std::multimap<int, int>{};
	for (auto seed = beg_seed; seed < end_seed; ++seed)
	{
		auto stamp = token::gen::Full(seed);
		auto rand = random::gen::Default(seed);
		auto encoder = Encoder(input, stamp, rand);
		auto decoder = Decoder(height, stamp);

		auto code = encoder.pop(height + redundancy);
		auto next = encoder.pop(1);
		auto check = [&] {
			return check_combinations(
				code, next,
				[&decoder, &height](const auto code) {
					decoder.clear();
					return decoder.push(code) == height;
				});
		};
		
		auto count = size_t{0};
		while (check())
		{
			next.push_back(encoder.pop(1));
			++count;
		}
		table.emplace(count, seed);
	}
	print_table(table);
	return 0;
}