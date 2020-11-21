/// @file  : encoder.hpp
/// @author: Luis Monteiro
/// @date  : Created on November 30, 2017, 5:48 PM

#pragma once

#include "container.hpp"
#include "engine.hpp"

namespace Codec {

/// Encoder
class Encoder {
  public:
    /// empty constructor
    Encoder() = default;

    /// constructor
    /// @param length
    /// @param capacity
    /// @param token
    Encoder(
      size_t length,
      size_t capacity,
      Token::Shared::Stamp token = Token::Default(Token::Type::FULL))
      : data_{}, capacity_{capacity}, token_{token} {}

    /// constructor
    /// @param data
    /// @param token
    Encoder(Container::Frames data, Token::Shared::Stamp token = Token::Default(Token::Type::FULL))
      : data_(std::move(data)), capacity_(data.size()), token_(token) {}

    /// Move Constructor
    Encoder(Encoder&&) = default;

    /// Move Operator
    Encoder& operator=(Encoder&&) = default;

    /// Data
    /// push data
    Encoder& push(Container::Frame data) {
        data_.push(std::move(data));
        return *this;
    }
    Encoder& push(Container::Frames data) {
        for (auto& d : data)
            data_.push(std::move(d));
        return *this;
    }

    /// pop data
    /// @param size
    auto pop(size_t size) { return Engine::Encode(*token_, data_, size); }

    /// clear data
    Encoder& clear() {
        data_.clear();
        return *this;
    }

    /// Iterators
    /// Forward
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    /// Quantity
    auto full() { return (data_.size() >= capacity_); }
    auto size() { return data_.size(); }
    auto capacity() { return capacity_; }

  private:
    /// data
    Container::Frames data_;

    /// context
    size_t capacity_;
    size_t size_;
    Token::Shared::Stamp token_;
};
} // namespace Codec
