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
        using Super = std::vector<uint8_t>;

      public:
        /// Default Constructors
        Frame()             = default;
        Frame(Frame&&)      = default;
        Frame(const Frame&) = default;

        /// Constructor
        /// @param vec
        Frame(Super vec)
          : Super(std::move(vec)) {}

        /// Constructor
        /// @param capacity
        /// @param size
        /// @param value
        Frame(size_t capacity, size_t size, uint8_t value)
          : Super() {
            reserve(capacity);
            assign(size, value);
        }

        /// Constructor
        /// @param capacity
        Frame(size_t capacity)
          : Super() {
            reserve(capacity);
        }

        /// Default Operators
        Frame& operator=(Frame&&) = default;
        Frame& operator=(const Frame&) = default;

        // Equal Operator
        template <typename Container>
        bool operator==(Container& cnt) {
            return (size() == cnt.size()) && std::equal(begin(), end(), cnt.begin());
        }

        /// Push
        /// @param data
        Frame& push(uint8_t data) {
            emplace_back(data);
            return *this;
        }

      protected:
        // protected methods
        using Super::emplace_back;
        using Super::push_back;
    };

    /// Exceptions
    class FramesException : public std::range_error {
      public:
        using std::range_error::range_error;
    };

    /// Frames
    class Frames : public std::vector<Frame> {
        using Super = std::vector<Frame>;

      public:
        /// Default Constructors
        Frames()              = default;
        Frames(Frames&&)      = default;
        Frames(const Frames&) = default;

        /// Constructor
        /// @param frame
        template <
            typename Container,
            typename = std::enable_if_t<std::is_assignable<Container, Frame>::value>>
        Frames(Container frame)
          : Super() {
            push(std::move(frame));
        }

        /// Constructor
        /// @param frames
        template <
            typename Container,
            typename Value = typename Container::value_type,
            typename       = std::enable_if_t<std::is_assignable<Value, Frame>::value>>
        Frames(Container frames)
          : Super() {
            push(std::move(frames));
        }

        /// Push
        /// @param frame
        template <
            typename Container,
            typename = std::enable_if_t<std::is_assignable<Container, Frame>::value>>
        Frames& push(Container frame) {
            emplace_back(std::move(frame));
            if (front().size() != back().size()) {
                pop_back();
                throw FramesException("unexpected frame size");
            }
            return *this;
        }

        /// Push Frames
        /// @param container
        template <
            typename Container,
            typename Value = typename Container::value_type,
            typename       = std::enable_if_t<std::is_assignable<Value, Frame>::value>>
        Frames& push(Container frames) {
            for (auto& f : frames)
                push(std::move(f));
            return *this;
        }

        // Equal Operator
        template <typename Container>
        bool operator==(Container& frames) {
            return (size() == frames.size()) && std::equal(begin(), end(), frames.begin());
        }

        // get frame size
        auto length() const { return at(0).size(); }

      protected:
        // protected methods
        using Super::emplace_back;
        using Super::push_back;
    };
} // namespace Container
} // namespace Codec

/// Utilities
static std::ostream& operator<<(std::ostream& os, const Codec::Container::Frame& f) {
    os << "[";
    for (auto v : f) {
        os << int(v) << " ";
    }
    return os << "]";
}
static std::ostream& operator<<(std::ostream& os, const Codec::Container::Frames& c) {
    os << "[" << std::endl;
    for (auto v : c) {
        os << " " << v << std::endl;
    }
    return os << "]";
}