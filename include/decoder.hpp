/// ===============================================================================================
/// @file      : decoder.hpp                                               |
/// @copyright : 2020 lcmonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================
#include "random.hpp"
#include "container.hpp"
#include "token.hpp"
#include "helpers/solver.hpp"
#include "helpers/gf8.hpp"
#include "helpers/combine.hpp"

namespace share::codec
{
/// encoder
/// @brief
template <
	class Frame,
	class Token,
	class Generator>
class Decoder
{
	using GF8 = share::codec::gf8::Space;

  public:
	using Data = Container<Frame>;
	template <class T>
	using frame = typename Data::template frame<T>;
	template <class T>
	using frames = typename Data::template frames<T>;

  public:
	/// constructor
	/// @param height
	/// @param token
	template <
		class T = token::Stamp,
		class R = random::Device,
		class G = random::gen::Default>
	Decoder(
		size_t height,
		T &&token = token::def::Full(),
		R &&rand = random::Device(),
		G &&gen = random::gen::Default())
		: token_{std::forward<T>(token)},
		  gen_{std::forward<G>(gen)},
		  data_{height},
		  coef_{},
		  capacity_{height},
		  size_{} {}

	Decoder(const Decoder &) = default;
	Decoder &operator=(const Decoder &) = default;
	Decoder(Decoder &&) = default;
	Decoder &operator=(Decoder &&) = default;

	/// push - decode process
	/// @param data
	template <class T>
	auto push(T &&data)
	{
		using helpers::make_coef;
		data_.push_back(
			data,
			[this](auto &frame) {
				coef_.push_back(
					capacity_,
					[this, &frame](auto&coef) {
						auto seed = pop_back<uint32_t>(frame);
						auto &dens = token_[seed];
						gen_.seed(seed);
						make_coef(gen_, dens, coef);
					});
			});
		size_ = helpers::solve<GF8>(
			capacity_,
			coef_.data(),
			data_.data());
		return size_;
	}

	auto pop()
	{
		auto tmp = std::move(data_);
		tmp.resize(size_);
		coef_ = Data{capacity_};
		data_ = Data{capacity_};
		size_ = 0;
		return tmp;
	}

  private:
	Token token_;
	Generator gen_;
	/// Cache
	Data data_;
	Data coef_;
	/// sizes
	size_t capacity_;
	size_t size_;
};
/// Deduction Guides
Decoder(size_t)
	->Decoder<std::vector<uint8_t>,
			  token::Stamp,
			  random::gen::Default>;
template <class T>
Decoder(size_t, T &&)
	->Decoder<std::vector<uint8_t>,
			  trait::remove_cvr_t<T>,
			  random::gen::Default>;
template <class T, class G>
Decoder(size_t, T &&, G &&)
	->Decoder<std::vector<uint8_t>,
			  trait::remove_cvr_t<T>,
			  trait::remove_cvr_t<G>>;
} // namespace share::codec
