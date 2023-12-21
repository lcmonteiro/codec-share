#include <algorithm>
#include <random>
#include <iomanip>
#include <iostream>

#include "decoder.hpp"
#include "encoder.hpp"

auto generate(size_t width, size_t height)
{
	std::random_device rnd;
	std::mt19937 engine{rnd()};
	std::uniform_int_distribution<uint8_t> dist{0, 255};
	std::vector<std::vector<uint8_t>> data;
	for (auto i = size_t{}; i < height; ++i)
	{
		std::vector<uint8_t> vec(width);
		std::generate(std::begin(vec), std::end(vec), [&dist, &engine]() {
			return dist(engine);
		});
		data.emplace_back(std::move(vec));
	}
	return data;
}

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

using namespace share::codec;
using Container = std::vector<uint8_t>;
using Encoder = share::codec::encoder<Container>;
using Decoder = share::codec::decoder<Container>;
int test_codec()
{
	int success_count = 0;
	for (auto seed = 1; seed < 10000; ++seed)
	{
		auto width = 1;
		auto height = 10;
		auto redundancy = 0;
		auto input = generate(width, height);
		auto token = token::generate(token::Type::FULL, seed);
		auto encoder = Encoder(input, token);
		auto code = encoder.pop(height + redundancy);
		auto decoder = Decoder(height, code, token);
		auto output = decoder.pop();
		print(input);
		print(code);
		print(output);

		success_count += int(input == output);
	}
	std::cout
		<< "success = " << std::dec << success_count
		<< std::endl;
	return 0;
}