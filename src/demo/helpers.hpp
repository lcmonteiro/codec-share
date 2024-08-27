#pragma once
#include <vector>
#include <sstream>
#include <iostream>
#include <map>
#include <algorithm>
#include "container.hpp"
using Frame = std::vector<uint8_t>;
using Image = share::codec::Container<Frame>;

const auto TO_CHAR = std::map<uint8_t, char>{
	{0x00, ' '},
	{0x01, '-'},
	{0x02, '/'},
	{0x03, '\\'},
	{0x04, '+'},
	{0x05, '|'},
	{0x06, '('},
	{0x07, ')'},
	{0x08, '['},
	{0x09, ']'},
	{0x0A, '_'},
	{0x0B, '.'},
	{0x0C, 'x'},
	{0x0D, '='},
	{0x0E, '*'},
	{0x0F, '#'}};
const auto TO_INT = std::map<char, uint8_t>{
	{' ', 0x0},
	{'-', 0x01},
	{'/', 0x02},
	{'\\', 0x03},
	{'+', 0x04},
	{'|', 0x05},
	{'(', 0x06},
	{')', 0x07},
	{'[', 0x08},
	{']', 0x09},
	{'_', 0x0A},
	{'.', 0x0B},
	{'x', 0x0C},
	{'=', 0x0D},
	{'*', 0x0E},
	{'#', 0x0F}};

inline auto image(const std::string &data)
{
	auto iss = std::istringstream{data};
	auto line = std::string{};
	auto result = Image{};
	while (std::getline(iss, line))
	{
		auto row = Frame{};
		auto ita = std::cbegin(line);
		auto itb = std::next(ita);
		auto end = std::end(line);
		while (ita != end and itb != end)
		{
			auto a = TO_INT.at(*ita);
			auto b = TO_INT.at(*itb);
			row.emplace_back((a << 4) | b);
			ita = std::next(itb);
			itb = std::next(ita);
		}
		result.push_back(std::move(row));
	}
	return result;
}

inline auto &operator<<(
	std::ostream& os, const Image &data)
{
	for (const auto &line : data.view())
	{
		for (const auto &value : line)
		{
			os << TO_CHAR.at(value >> 4);
			os << TO_CHAR.at(value & 7);
		}
		os << std::endl;
	}
	return os;
}