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
    // empty constructor
    Decoder() = default;

    /// move constructor
    Decoder(Decoder&&) = default;

    /// constructor
    /// @param capacity
    /// @param token
    Decoder(
        Size capacity,
        Token::Shared::Key token = Token::Default(Token::Type::FULL))
      : data_{}
      , coef_{}
      , field_(capacity << 1)
      , token_(token)
      , capacity_(capacity)
      , size_{} {
        coef_.reserve(capacity << 1);
        data_.reserve(capacity << 1);
    }

    /// constructor
    /// @param capacity
    /// @param init
    /// @param token
    Decoder(
        Size capacity,
        Container::Frames init,
        Token::Shared::Key token = Token::Default(Token::Type::FULL))
      : Decoder(capacity, token) {
        push(std::move(init));
    }

    /// move operator
    Decoder& operator=(Decoder&&) = default;

    /// Push
    /// @param data
    Decoder& push(Container::Frame data) {
        size_ = Engine::Decode(
            *token_,
            Container::Frames{std::move(data)},
            capacity_,
            data_,
            coef_,
            field_);
        return *this;
    }

    /// Push
    /// @param data
    Decoder& push(Container::Frames data) {
        size_ = Engine::Decode(
            *token_, std::move(data), capacity_, data_, coef_, field_);
        return *this;
    }

    /// Pop data
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
    Decoder& clear() {
        size_ = 0;
        coef_.clear();
        data_.clear();
        field_.clear();
        return *this;
    }

    /// Iterators
    /// Forward
    auto begin() const { return data_.begin(); }
    auto end() const { return std::next(data_.begin(), size_); }

    /// Backward
    auto rbegin() const { return std::prev(data_.rend(), size_); }
    auto rend() const { return data_.rend(); }

    /// References
    auto front() const { return data_.front(); }
    auto at(size_t n) const { return data_.at(n); }
    auto back() const { return data_.at(size_ - 1); }

    /// Quantity
    bool full() { return (size_ >= capacity_); }
    bool empty() { return (size_ == 0); }
    Size size() { return size_; }
    Size capacity() { return capacity_; }
    void resize(Size size) {
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
    Token::Shared::Key token_;
    Size capacity_;
    Size size_;
};
} // namespace Codec
