/// @file engine.hpp
/// @author Luis Monteiro
/// @date  November 23, 2016, 10:21 AM

#pragma once

#include <stdexcept>
#include <iostream>
#include <vector>

namespace Codec {
namespace Container {

    class FrameException : public std::range_error {
      public:
        using std::range_error::range_error;
    };

    class Frame : public std::vector<uint8_t> {
      public:
        /// Constructor
        /// @param capacity
        /// @param size
        /// @param value
        Frame(const size_t capacity, const size_t size, const uint8_t value)
          : std::vector<uint8_t>() {
            reserve(capacity);
            assign(size, value);
        }

        /// Constructor
        /// @param capacity
        Frame(const size_t capacity)
          : std::vector<uint8_t>() {
            reserve(capacity);
        }

        /// Default Constructors
        Frame()             = default;
        Frame(Frame&&)      = default;
        Frame(const Frame&) = default;

        /// Default Operators
        Frame& operator=(Frame&&) = default;
        Frame& operator=(const Frame&) = default;

        /// operator ==
        /// @param c container under comparation
        template <typename Container>
        bool operator==(const Container& c) const {
            return std::equal(begin(), end(), c.begin(), c.end());
        }

        /// Shrink to size -> resize
        ///  - throw exception when size > size()

        Frame& shrink(size_t sz = 0) {
            if (size() < sz)
                throw FrameException("Shrink");
            resize(sz);
            return *this;
        }

        /// Inflate
        Frame& inflate() {
            resize(capacity());
            return *this;
        }

        /// check size
        bool empty() const { return (size() == 0); }
        bool full() const { return (size() == capacity()); }
    };

    std::ostream& operator<<(std::ostream& os, const Frame& f) {
        os << "[";
        for (auto v : f) {
            os << int(v) << " ";
        }
        return os << "]";
    }

    /// Exceptions
    class FramesException : public std::range_error {
      public:
        using std::range_error::range_error;
    };

    /// Frames
    class Frames : public std::vector<Frame> {
      public:
        using Super = std::vector<Frame>;

        /// Using Supper
        using Super::Super;
        using Super::operator=;

        /// Constructors
        Frames(std::size_t capacity)
          : Super() {
            reserve(capacity);
        }

        /// Clear Frames
        Frames& clear() {
            Super::clear();
            return *this;
        }

        /// Push a frame Frames
        template <typename Frames>
        Frames& push(const Frames& c) {
            insert(end(), c.begin(), c.end());
            return *this;
        }

        /// Check size
        bool full() const { return (size() == capacity()); }
    };

    /// Utilities
    std::ostream& operator<<(std::ostream& os, const Frames& c) {
        os << "[" << std::endl;
        for (auto v : c) {
            os << " " << v << std::endl;
        }
        return os << "]";
    }
} // namespace Container
} // namespace Codec