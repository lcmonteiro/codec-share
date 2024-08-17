#include <algorithm>
#include <random>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "decoder.hpp"
#include "encoder.hpp"
#include "container.hpp"

static auto generate(size_t width, size_t height)
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

static auto print = [](const auto &matrix) {
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
using namespace std::chrono;
using Frame = std::vector<uint8_t>;

int test_codec(
	int n_seeds,
	int width,
	int height = 40,
	int redundancy = 0)
{
	auto success_count = 0;
	auto start_time = high_resolution_clock::now();

	for (auto seed = 0; seed < n_seeds; ++seed)
	{
		auto stamp = token::gen::Full(seed);
		auto rand = random::gen::Default(seed);

		auto input = generate(width, height);
		//std::cout << "input =" << std::endl;
		//print(input);
		auto encoder = Encoder(input, stamp,rand);
		auto code = encoder.pop(height + redundancy);
		//std::cout << "code =" << std::endl;
		//print(code);
		auto decoder = Decoder(height, stamp);
		decoder.push(code);
		auto output = decoder.pop();
		//std::cout << "output=" << std::endl;
		//print(output);
		success_count += int(input == output.view());
	}
	auto stop_time = high_resolution_clock::now();
	auto delta_time =
		duration<double, std::milli>{stop_time - start_time};
	std::cout
		<< "success = " << std::dec << success_count
		<< std::endl
		<< "duration = " << delta_time.count()
		<< std::endl;
	return 0;
}