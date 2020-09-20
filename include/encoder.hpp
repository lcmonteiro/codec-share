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
    /// @param capacity
    /// @param token
    Encoder(
        Size capacity,
        Token::Shared::Key token = Token::Default(Token::Type::FULL))

      : data_(capacity)
      , capacity_(capacity)
      , size_(capacity)
      , token_(token) {}

    /// constructor
    /// @param init
    /// @param token
    Encoder(
        Container::Frames&& init,
        Token::Shared::Key token = Token::Default(Token::Type::FULL))
      : data_(std::move(init))
      , capacity_(init.size())
      , size_(init.size())
      , token_(token) {}

    /// Move Constructor
    Encoder(Encoder&&) = default;

    /// Move Operator
    Encoder& operator=(Encoder&&) = default;

    /// Data
    /// push data
    Encoder& push(Container::Frame data) {
        data_.emplace_back(std::move(data));
        return *this;
    }
    Encoder& push(Container::Frames data) {
        for (auto& d : data)
            data_.emplace_back(std::move(d));
        return *this;
    }

    /// pop data
    auto pop() {
        Container::Frames out;
        Engine::Encode(*token_, data_, size_, out);
        return std::move(out);
    }

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
    bool full() { return (data_.size() >= capacity_); }
    Size size() { return data_.size(); }
    Size capacity() { return capacity_; }

  private:
    /// data
    Container::Frames data_;

    /// context
    Size capacity_;
    Size size_;
    Token::Shared::Key token_;
};
} // namespace Codec