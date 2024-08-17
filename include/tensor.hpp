/// ===============================================================================================
/// @file      : tensor.hpp                                             |
/// @copyright : 2024 lcmonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <stdexcept>
#include <vector>

namespace share::codec
{
template <typename T>
class container
{
	using value_type = typename T::value_type;

	// required
	static_assert(std::is_same_v<value_type, uint8_t>);

  public:
	/// exceptions
	struct exception : std::range_error
	{
		using std::range_error::range_error;
	};

	/// default constructors
	container() = default;
	container(container &&) = default;
	container(const container &) = default;
	/// default assigns
	container &operator=(container &&) = default;
	container &operator=(const container &) = default;

	/// constructor
	/// @param frame
	template <typename Container>
	container(Container init)
	{
		this->push_back(std::move(init));
	}

	/// push back
	/// @param container
	template <typename Container>
	std::enable_if_t<std::is_same_v<Vector, std::decay_t<Container>>, void>
	push_back(Container &&container)
	{
		this->emplace_back(std::forward<Container>(container));
		if (this->front().size() != this->back().size())
		{
			this->pop_back();
			throw exception("unexpected container size");
		}
	}

	/// push back
	/// @param container
	template <typename Container>
	std::enable_if_t<std::is_same_v<Vector, typename std::decay_t<Container>::value_type>, void>
	push_back(Container container)
	{
		for (auto &vec : container)
			push_back(std::move(vec));
	}

	/// length
	/// @return vector size
	auto size() const
	{
		return at(0).size();
	}

  private:
	using std::vector<Vector>::emplace_back;
	using std::vector<Vector>::emplace;
	using std::vector<Vector>::insert;
};
} // namespace share::codec

/// Utilities
template <typename Vector>
inline auto &operator<<(
	std::ostream &os,
	const share::codec::tensor<Vector> &max)
{
	os << "[";
	for (auto vec : max)
	{
		os << std::endl;
		for (auto val : vec)
			os << " " << int(val);
	}
	os << std::endl
	   << "]";
	return os;
}
