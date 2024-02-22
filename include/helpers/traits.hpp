/// ===============================================================================================
/// @file      : traits.hpp                                                |
/// @copyright : 2024 lcmonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <type_traits>

namespace share::codec
{
namespace trait
{
namespace detail
{
template <bool test>
using enable = std::enable_if_t<test, int>;
}

/// is traits
template <class A, class B>
inline constexpr bool is_equal =
	std::is_same_v<
		std::decay_t<A>,
		std::decay_t<B>>;
template <class A, class B>
inline constexpr bool is_convertible =
	std::is_convertible_v<
		std::decay_t<A>,
		std::decay_t<B>>;
template <class T, class A>
inline constexpr bool is_constructible =
	std::is_constructible_v<
		std::decay_t<T>,
		std::decay_t<A>>;
		
/// if traits
template <bool... test>
using if_all =
	detail::enable<(test and ...)>;
template <bool... test>
using if_any =
	detail::enable<(test or ...)>;
template <class A, class B>
using if_equal =
	detail::enable<is_equal<A, B>>;
template <class A, class B>
using if_convertible =
	detail::enable<is_convertible<A, B>>;
template <class T, class A>
using if_constructible =
	detail::enable<is_constructible<T, A>>;
	
/// type traits
template <class T>
using remove_cvr_t =
	std::remove_reference_t<std::remove_cv_t<T>>;
template <class T>
using value_t =
	typename std::remove_reference_t<T>::value_type;
} // namespace trait
} // namespace share::codec
