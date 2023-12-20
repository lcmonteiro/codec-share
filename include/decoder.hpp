/// ===============================================================================================
/// @file      : decoder.hpp                                               |
/// @copyright : 2020 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include "container.hpp"
#include "helpers/solve.hpp"
#include "token.hpp"

namespace share::codec
{
/// definitions
using Random = std::random_device;
using Generator = std::minstd_rand0;

/// decoder
/// @brief
template <typename Vector>
class decoder
{
  public:
	// helpers
	using Container = container<Vector>;
	using Value = typename Vector::value_type;

	/// empty constructor
	decoder() = default;

	/// move constructor
	decoder(decoder &&) = default;

	/// constructor
	/// @param capacity
	/// @param token
	decoder(size_t capacity, token::shared::Stamp token = token::get(token::Type::FULL))
		: data_{}, coef_{}, field_{}, capacity_{capacity}, size_{}, token_{token}
	{
		coef_.reserve(capacity << 1);
		data_.reserve(capacity << 1);
		field_.reserve(capacity << 1);
	}

	/// constructor
	/// @param capacity
	/// @param init
	/// @param token
	decoder(size_t capacity, Container init, token::shared::Stamp token)
		: decoder(capacity, token)
	{
		push(std::move(init));
	}

	/// constructor
	/// @param data
	/// @param token
	decoder(Container data, token::shared::Stamp token = token::get(token::Type::FULL))
		: decoder(data.size(), token)
	{
		push(std::move(data));
	}

	/// move operator
	decoder &operator=(decoder &&) = default;

	/// push
	/// @param data
	void push(Container data);

	/// push
	/// @param data
	void push(Vector data) { push(Container{std::move(data)}); }

	/// pop
	/// @return decoded frames
	Container pop()
	{
		data_.resize(size_);
		size_ = 0;
		coef_.clear();
		field_.clear();
		return std::move(data_);
	}

	/// Clear data
	/// @return this
	void clear()
	{
		size_ = 0;
		coef_.clear();
		data_.clear();
		field_.clear();
	}

	/// Iterators
	/// forward
	auto begin() const { return std::begin(data_); }
	auto end() const { return std::next(std::begin(data_), size_); }

	/// backward
	auto rbegin() const { return std::prev(std::rend(data_), size_); }
	auto rend() const { return std::rend(data_); }

	/// references
	auto &front() const { return data_.front(); }
	auto &at(size_t n) const { return data_.at(n); }
	auto &back() const { return data_.at(size_ - 1); }

	/// quantity
	auto full() { return (size_ >= capacity_); }
	auto empty() { return (size_ == 0); }
	auto size() { return size_; }
	auto capacity() { return capacity_; }
	void resize(size_t size)
	{
		data_.resize(size);
		coef_.resize(size);
		field_.resize(size);
		if (size_ > size)
			size_ = size;
	}

  private:
	/// Cache
	Container data_;
	Container coef_;
	std::vector<Value> field_;

	/// Context
	size_t capacity_;
	size_t size_;

	/// Property
	token::shared::Stamp token_;
};

/// push
/// @param data
template <typename Vector>
void decoder<Vector>::push(Container data)
{
	const auto& token = *token_;
	for (auto &frame : data)
	{
		// remove seed
		auto seed = uint32_t(frame.back());
		frame.pop_back();
		seed <<= 8;
		seed |= uint32_t(frame.back());
		frame.pop_back();
		seed <<= 8;
		seed |= uint32_t(frame.back());
		frame.pop_back();
		seed <<= 8;
		seed |= uint32_t(frame.back());
		frame.pop_back();
		// properties
		auto [field, sparse] = token[uint8_t(seed)];
		auto generator = Generator{seed};
		// gerenate coefficients
		auto coef = Vector(capacity_ + sizeof(int));
		for (auto &val : coef)
		{
			auto factor = Value(generator());
			if (factor > sparse)
				continue;
			val = (factor & field);
		}
		data_.push_back(std::move(frame));
		coef_.push_back(std::move(coef));
		field_.push_back(field);
	}
	size_ = helpers::solve(capacity_, field_, coef_, data_);
}
} // namespace share::codec
