/// ===============================================================================================
/// @file      : encoder.hpp                                               |
/// @copyright : 2020 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================
#pragma once

#include "random.hpp"
#include "container.hpp"
#include "token.hpp"
#include "helpers/combine.hpp"

namespace share::codec
{
/// encoder
/// @brief
template <
	typename Frame,
	typename Token,
	typename Random,
	typename Generator>
class Encoder : Container<Frame>
{
  public:
	using Data = Container<Frame>;

	/// encode header size
	const size_t HEADER_SIZE = sizeof(uint32_t);

	/// constructor
	/// @param init
	/// @param token
	template <
		class C,
		class T = token::Stamp,
		class R = random::Device,
		class G = random::gen::Default>
	Encoder(
		C &&init,
		T &&token = token::def::Full(),
		R &&rand = random::Device(),
		G &&gen = random::gen::Default())
		: Data{std::forward<C>(init)},
		  token_{std::forward<T>(token)},
		  rand_{std::forward<R>(rand)},
		  gen_{std::forward<G>(gen)} {}

	Encoder(const Encoder &) = default;
	Encoder &operator=(const Encoder &) = default;
	Encoder(Encoder &&) = default;
	Encoder &operator=(Encoder &&) = default;

	/// pop - encode process
	auto pop(size_t);

  private:
	Token token_;
	Random rand_;
	Generator gen_;
};

/// pop
/// @param size
/// @return data
template <
	typename Frame,
	typename Token,
	typename Random,
	typename Generator>
auto Encoder<
	Frame,
	Token,
	Random,
	Generator>::pop(size_t size)
{
	using helpers::make_comb;
	// containers
	auto data = this->view();
	auto code = Data{size};
	// sizes
	auto data_width = this->width();
	auto code_width = data_width + HEADER_SIZE;
	// encode loop
	while (size--)
	{
		code.push_back(
			code_width,
			[&](auto &comb) {
				// create combination
				auto seed = rand_();
				auto &dens = token_[seed];
				comb.resize(data_width);
				for (
					gen_.seed(seed);
					make_comb(gen_, data, dens, comb) == 0;
					gen_.seed(seed))
					;
				// insert seed
				push_back(comb, seed);
			});
	}
	return code;
}

/// Deduction Guides
template <class C>
Encoder(C &&)
	->Encoder<trait::value_t<C>,
			  token::Stamp,
			  random::Device,
			  random::gen::Default>;
template <class C, class T>
Encoder(C &&, T &&)
	->Encoder<trait::value_t<C>,
			  trait::remove_cvr_t<T>,
			  random::Device,
			  random::gen::Default>;
template <class C, class T, class R>
Encoder(C &&, T &&, R &&)
	->Encoder<trait::value_t<C>,
			  trait::remove_cvr_t<T>,
			  trait::remove_cvr_t<R>,
			  random::gen::Default>;
template <class C, class T, class R, class G>
Encoder(C &&, T &&, R &&, G &&)
	->Encoder<trait::value_t<C>,
			  trait::remove_cvr_t<T>,
			  trait::remove_cvr_t<R>,
			  trait::remove_cvr_t<G>>;
} // namespace share::codec
