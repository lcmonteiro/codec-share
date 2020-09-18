/// @file decoder.hpp
/// @author Luis Monteiro
/// @date November 30, 2015, 5:48 PM

#pragma once

#include "container.hpp"
#include "engine.hpp"
namespace Codec {

/// Codec Decoder
class Decoder {
  public:
    /// move constructor
    Decoder(Decoder&&) = default;

    // empty constructor
    Decoder()
      : data_()
      , coef_()
      , field_()
      , stamp_(SStamp::Get(SStamp::FULL))
      , capacity_(0)
      , size_(0)
      , rand_() {}

    /// constructor
    /// @param capacity
    /// @param stamp
    Decoder(Size capacity, pStamp stamp = SStamp::Get(SStamp::FULL))
      : data_()
      , coef_()
      , field_(capacity << 1)
      , stamp_(stamp)
      , capacity_(capacity)
      , size_(0)
      , rand_() {
        coef_.reserve(capacity << 1);
        data_.reserve(capacity << 1);
    }

    /// constructor
    /// @param capacity
    /// @param init
    /// @param stamp
    Decoder(
        Size capacity,
        Container&& init,
        pStamp stamp = SStamp::Get(SStamp::FULL))
      : Decoder(capacity, stamp) {
        push(std::move(init));
    }

    /// move operator
    Decoder& operator=(Decoder&&) = default;

    /// Push
    /// @param data
    Decoder& push(Frame data) {
        size_ = Engine::Decode(
            Container({std::move(data)}),
            capacity_,
            data_,
            coef_,
            field_,
            *stamp_);
        return *this;
    }

    /// Push
    /// @param data
    Decoder& push(Container data) {
        size_ = Engine::Decode(
            std::move(data), capacity_, data_, coef_, field_, *stamp_);
        return *this;
    }

    /// Pop data
    Container pop() {
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
    using const_iterator         = Container::const_iterator;
    using const_reverse_iterator = Container::const_reverse_iterator;
    using const_reference        = Container::const_reference;

    /// Forward
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return std::next(data_.begin(), size_); }

    /// Backward
    const_reverse_iterator rbegin() const {
        return std::prev(data_.rend(), size_);
    }
    const_reverse_iterator rend() const { return data_.rend(); }

    /// References
    const_reference front() const { return data_.front(); }
    const_reference at(size_t n) const { return data_.at(n); }
    const_reference back() const { return data_.at(size_ - 1); }

    /// Quantity
    inline bool full() { return (size_ >= capacity_); }
    inline bool empty() { return (size_ == 0); }
    inline uint32_t size() { return size_; }
    inline uint32_t capacity() { return capacity_; }
    inline void resize(uint32_t size) {
        data_.resize(size);
        coef_.resize(size);
        field_.resize(size);
        if (size_ > size)
            size_ = size;
    }

  private:
    /// Cache
    Container data_;
    Container coef_;
    Frame field_;

    /// Context
    pStamp stamp_;
    Size capacity_;
    Size size_;

    /// seed generator
    Random rand_;
};
} // namespace Codec
