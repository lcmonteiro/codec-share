/// ===============================================================================================
/// @file      : encoder.hpp                                               |
/// @copyright : 2020 LCMonteiro                                     __|   __ \    _` |   __|  _ \ 
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <random>

#include "container.hpp"
#include "token.hpp"

#include "detail/combine.hpp"

namespace codec {

/// encoder
/// @brief
template <typename Vector, typename Random = std::random_device, typename Generator = std::minstd_rand0>
class encoder {
  public:
    // helpers
    using Container = container<Vector>;
    using Value     = typename Vector::value_type;

    /// encode header size
    const size_t HEADER_SIZE = sizeof(uint32_t);

    /// constructor
    /// @param capacity
    /// @param token
    encoder(size_t capacity = 100, token::shared::Stamp token = token::get(token::Type::FULL))
      : data_{}, capacity_{capacity}, token_{token} {}

    /// constructor
    /// @param data
    /// @param token
    encoder(Container data, token::shared::Stamp token = token::get(token::Type::FULL))
      : data_(std::move(data)), capacity_(data.size()), token_(token) {}

    /// move constructor
    encoder(encoder&&) = default;

    /// move operator
    encoder& operator=(encoder&&) = default;

    /// push
    /// @param data
    void push(Vector data) { data_.push(std::move(data)); }

    /// push
    /// @param data
    void push(Container data) {
        for (auto& d : data)
            data_.push(std::move(d));
    }

    /// pop
    /// @param size
    auto pop(size_t size);

    /// clear
    void clear() { data_.clear(); }

    /// iterators
    /// @brief forward
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    /// quantity
    auto full() { return (data_.size() >= capacity_); }
    auto size() { return data_.size(); }
    auto capacity() { return std::max(capacity_, data_.size()); }

  private:
    /// data
    Container data_;
    /// context
    size_t capacity_;
    // property
    token::shared::Stamp token_;
};


/// push
/// @param size
/// @return data
template <typename Vector, typename Random, typename Generator>
auto encoder<Vector, Random, Generator>::pop(size_t size) {
    // coded container
    Container code;
    // random generetor
    Random rand;
    // sizes
    auto data_length = data_.length();
    auto code_length = data_length + HEADER_SIZE;
    // encode loop
    for (unsigned int i = 0; i < size; i++) {
        // seed, field and sparsity
        auto seed     = uint32_t(0);
        auto field    = uint8_t(0);
        auto sparsity = uint8_t(0);

        // create combination
        auto comb = Vector(code_length + sizeof(int));
        comb.resize(data_length);
        do {
            seed     = rand();
            field    = (*token_)[uint8_t(seed)].first;
            sparsity = (*token_)[uint8_t(seed)].second;
        } while (detail::combine<Generator>(data_, seed, field, sparsity, comb) == 0);

        // insert seed
        comb.push_back(uint8_t(seed));
        seed >>= 8;
        comb.push_back(uint8_t(seed));
        seed >>= 8;
        comb.push_back(uint8_t(seed));
        seed >>= 8;
        comb.push_back(uint8_t(seed));
        
        // save combination
        code.push_back(std::move(comb));
    }
    return code;
}
} // namespace codec
