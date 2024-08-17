/// ===============================================================================================
/// @file      : Container.hpp                                             |
/// @copyright : 2019 lcnonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================
#pragma once
#include <stdexcept>
#include <vector>

#include "helpers/traits.hpp"

namespace share::codec
{
/// concepts
template <class T>
using if_frame =
	trait::if_equal<
		uint8_t, trait::value_t<std::decay_t<T>>>;

template <class Frame, if_frame<Frame> = 0>
class Container : protected std::vector<Frame>
{
	using Data = std::vector<Frame>;

  public:
	/// concepts
	template <class T>
	using frame =
		trait::if_constructible<Frame,T>;
	template <class T>
	using frames =
		trait::if_convertible<
			Frame, trait::value_t<std::decay_t<T>>>;

	///
	using value_type = typename Data::value_type;

	/// exceptions
	struct RangeError : std::range_error
	{
		using std::range_error::range_error;
	};

	/// default constructors
	Container() = default;
	Container(Container &&) = default;
	Container(const Container &) = default;
	/// default assigns
	Container &operator=(Container &&) = default;
	Container &operator=(const Container &) = default;

	/// constructor
	/// @param init container like
	template <class T, frames<T> = 0>
	Container(T &&init)
		: Data{std::forward<T>(init)} {}
	/// constructor and reserve
	/// @param height
	Container(size_t height)
		: Data{} { this->reserve(height); }

	/// push back
	/// @param data frame
	template <class T, frame<T> = 0>
	void push_back(T &&data)
	{
		this->emplace_back(std::forward<T>(data));
		if (this->front().size() != this->back().size())
		{
			this->pop_back();
			throw RangeError("unexpected size");
		}
	}
	/// push back
	/// @param data frame
	template <class T, class F, frame<T> = 0>
	void push_back(T &&data, const F &format)
	{
		this->emplace_back(std::forward<T>(data));
		format(this->back());
		if (this->front().size() != this->back().size())
		{
			this->pop_back();
			throw RangeError("unexpected size");
		}
	}
	/// push back
	/// @param container
	template <class T, frames<T> = 0>
	void push_back(T &&data)
	{
		for (auto &&vec : data)
			push_back(vec);
	}
	template <class T, class F, frames<T> = 0>
	void push_back(T &&data, const F &format)
	{
		for (auto &&vec : data)
			push_back(vec, format);
	}
	/// shape
	auto width() const
	{
		return this->at(0).size();
	}
	auto height() const
	{
		return this->size();
	};
	auto &view() const
	{
		return static_cast<const Data &>(*this);
	}

  protected:
  template <class F, class T, class R, class G>
	friend class Encoder;
	template <class F, class T, class G>
	friend class Decoder;
	auto &data()
	{
		return static_cast<Data &>(*this);
	}
};

/// operations
template <class T, class F>
auto pop_back(F &frame)
{
	// extract seed
	auto seed = T(frame.back());
	frame.pop_back();
	seed <<= 8;
	if constexpr (sizeof(T) == 1)
		return seed;
	seed |= T(frame.back());
	frame.pop_back();
	seed <<= 8;
	if constexpr (sizeof(T) == 2)
		return seed;
	seed |= T(frame.back());
	frame.pop_back();
	seed <<= 8;
	seed |= T(frame.back());
	frame.pop_back();
	if constexpr (sizeof(T) == 4)
		return seed;
	static_assert(true, "pop_back: failed");
}
template <class F, class T>
void push_back(F &frame, T value)
{
	frame.push_back(uint8_t(value));
	if constexpr (sizeof(T) == 1)
		return;
	value >>= 8;
	frame.push_back(uint8_t(value));
	if constexpr (sizeof(T) == 2)
		return;
	value >>= 8;
	frame.push_back(uint8_t(value));
	value >>= 8;
	frame.push_back(uint8_t(value));
	if constexpr (sizeof(T) == 4)
		return;
	static_assert(true, "push_back: failed");
}
} // namespace share::codec
