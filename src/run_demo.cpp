#include "decoder.hpp"
#include "encoder.hpp"
#include "container.hpp"
#include "demo/input.hpp"
#include "demo/helpers.hpp"

using namespace share::codec;
int run_demo()
{
	auto seed = size_t{};
	auto height = size_t{4};
	auto capacity = size_t{10};
	auto redundancy = size_t{0};
	auto stamp = token::gen::Full(seed);
	auto rand = random::gen::Default<uint8_t>(seed);
	auto input = image(input_str);
	std::cout << "input:" << std::endl;
	std::cout << input << std::endl;
	auto encoder = Encoder(input, stamp, rand);
	auto code1 = encoder.pop(height + redundancy);
	std::cout << "code1:" << std::endl;
	std::cout << code1 << std::endl;
	auto code2 = encoder.pop(height + redundancy);
	std::cout << "code2:" << std::endl;
	std::cout << code2 << std::endl;
	auto decoder = Decoder(capacity, stamp);
	decoder.push(code1);
	decoder.push(code2);
	auto output = decoder.pop();
	std::cout << "output:" << std::endl;
	std::cout << output << std::endl;
	return 0;
}