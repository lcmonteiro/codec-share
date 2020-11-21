/// @file   : decoder.hpp
/// @author : Luis Monteiro
/// @date   : November 30, 2015, 5:48 PM

#pragma once

#include "container.hpp"
#include "engine.hpp"
namespace Codec {

/// Codec Decoder
class Decoder {
  public:
    /// empty constructor
    Decoder() = default;

    /// move constructor
    Decoder(Decoder&&) = default;

    /// constructor
    /// @param capacity
    /// @param token
    Decoder(size_t capacity, Token::Shared::Stamp token = Token::Default(Token::Type::FULL))
      : data_{}, coef_{}, field_(capacity << 1), token_(token), capacity_(capacity), size_{} {
        coef_.reserve(capacity << 1);
        data_.reserve(capacity << 1);
    }

    /// constructor
    /// @param capacity
    /// @param init
    /// @param token
    Decoder(
      size_t capacity,
      Container::Frames init,
      Token::Shared::Stamp token = Token::Default(Token::Type::FULL))
      : Decoder(capacity, token) {
        push(std::move(init));
    }

    /// constructor
    /// @param data
    /// @param token
    Decoder(Container::Frames data, Token::Shared::Stamp token = Token::Default(Token::Type::FULL))
      : Decoder(data.size(), token) {
        push(std::move(data));
    }

    /// move operator
    Decoder& operator=(Decoder&&) = default;

    /// push
    /// @param data
    Decoder& push(Container::Frame data) {
        size_ = Engine::Decode(*token_, {std::move(data)}, capacity_, data_, coef_, field_);
        return *this;
    }

    /// push
    /// @param data
    Decoder& push(Container::Frames data) {
        size_ = Engine::Decode(*token_, std::move(data), capacity_, data_, coef_, field_);
        return *this;
    }

    /// pop
    /// @return decoded frames
    Container::Frames pop() {
        // move data
        auto out{std::move(data_)};
        // update size
        out.resize(size_);
        // reset
        clear();
        return out;
    }

    /// Clear data
    /// @return this
    Decoder& clear() {
        size_ = 0;
        coef_.clear();
        data_.clear();
        field_.clear();
        return *this;
    }

    /// Iterators
    /// forward
    auto begin() const { return data_.begin(); }
    auto end() const { return std::next(data_.begin(), size_); }

    /// backward
    auto rbegin() const { return std::prev(data_.rend(), size_); }
    auto rend() const { return data_.rend(); }

    /// references
    auto front() const { return data_.front(); }
    auto at(size_t n) const { return data_.at(n); }
    auto back() const { return data_.at(size_ - 1); }

    /// quantity
    auto full() { return (size_ >= capacity_); }
    auto empty() { return (size_ == 0); }
    auto size() { return size_; }
    auto capacity() { return capacity_; }
    void resize(size_t size) {
        data_.resize(size);
        coef_.resize(size);
        field_.resize(size);
        if (size_ > size)
            size_ = size;
    }

  private:
    /// Cache
    Container::Frames data_;
    Container::Frames coef_;
    Container::Frame field_;

    /// Context
    Token::Shared::Stamp token_;
    size_t capacity_;
    size_t size_;
};
} // namespace Codec
