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
	size_t operator()() { return dev_(); }

  private:
	std::random_device dev_;
};

namespace gen
{
template <class Seed = std::uint32_t>
struct Default
{
	using seed_t = Seed;
	Default() = default;
	Default(const Default &) = default;
	Default(Seed value) : gen_(value) {}

	void seed(Seed value) { gen_.seed(value); }
	auto operator()() { return Seed(gen_()); }

  private:
	std::minstd_rand0 gen_;
};
} // namespace gen
} // namespace random
} // namespace share::codec
