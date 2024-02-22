/// ===============================================================================================
/// @file      : random.hpp                                                 |
/// @copyright : 2024 lcmonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================
#pragma once
#include <random>

namespace share::codec
{
namespace random
{
struct Device
{
	Device() = default;
	Device(const Device &) {}
	auto operator()() { return dev_(); }

  private:
	std::random_device dev_;
};

namespace gen
{
using Default = std::minstd_rand0;
}
} // namespace random
} // namespace share::codec
