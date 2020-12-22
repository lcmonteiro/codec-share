/// @file container.hpp
/// @copyright
///
/// @author Luis Monteiro

#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>

namespace codec {

/// Container
template <typename Vector>
class container : public std::vector<Vector> {
    // container concepts
    static_assert(std::is_same_v<typename Vector::value_type, uint8_t>);

  public:
    /// exceptions
    class exception : public std::range_error {
      public:
        using std::range_error::range_error;
    };

    /// default constructors
    container()                 = default;
    container(container&&)      = default;
    container(const container&) = default;
    /// default assigns
    container& operator=(container&&) = default;
    container& operator=(const container&) = default;

    /// constructor
    /// @param frame
    template <typename Container>
    container(Container init) {
        this->push_back(std::move(init));
    }

    /// push back
    /// @param container
    template <typename Container>
    std::enable_if_t<std::is_assignable_v<Vector&, std::decay_t<Container>>, void>
    push_back(Container&& container) {
        this->emplace_back(std::forward<Container>(container));
        if (this->front().size() != this->back().size()) {
            this->pop_back();
            throw exception("unexpected container size");
        }
    }

    /// push back
    /// @param container
    template <typename Container>
    std::enable_if_t<std::is_assignable_v<Vector&, typename std::decay_t<Container>::value_type>, void>
    push_back(Container container) {
        for (auto& vec : container)
            push_back(std::move(vec));
    }

    /// length 
    /// @return vector size
    auto length() const { return this->at(0).size(); }

  private:
    using std::vector<Vector>::emplace_back;
    using std::vector<Vector>::emplace;
    using std::vector<Vector>::insert;
};
} // namespace codec

/// Utilities
template <typename Vector>
static std::ostream& operator<<(std::ostream& os, const codec::container<Vector>& max) {
    os << "[";
    for (auto vec : max) {
        os << std::endl;
        for (auto val : vec)
            os << " " << int(val);
    }
    return os << std::endl << "]";
}
