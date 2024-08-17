/// ===============================================================================================
/// @file      : stream.hpp                                                |
/// @copyright : 2019 lcmonteiro                                     __|   __ \    _` |   __|  _ \. 
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include "decoder.hpp"
#include "encoder.hpp"

#include "helpers/copy.hpp"

namespace share::codec {

/// ===============================================================================================
/// istream
/// @brief
/// ===============================================================================================
template <typename Vector = std::vector<uint8_t>, typename Size = uint32_t>
class istream {
    static constexpr int DEFAULT_CAPACITY = 100;

  public:
    /// constructor
    /// @param capacity
    /// @param token
    explicit istream(token::shared::Stamp token = token::get(token::Type::FULL))
      : encoder_{DEFAULT_CAPACITY, token} {}

    /// set
    /// @param data
    /// @param framesize
    /// @param redundancy
    Size set(const Vector& data, Size framesize, Size redundancy = 0) {
        // helpers
        Size size  = std::max(framesize - encoder_.HEADER_SIZE, sizeof(Size));
        auto frame = Vector(size, 0);

        // add head information
        auto fit = helpers::copy(Size(data.size()), std::begin(frame));
        auto dit = std::next(std::begin(data), std::min(data.size(), size - sizeof(size)));

        // split data and add to encoder
        if (std::copy(std::begin(data), dit, fit) == std::end(frame)) {
            encoder_.push(frame);
            for (std::advance(dit, size); dit < std::end(data); std::advance(dit, size))
                encoder_.push(Vector(prev(dit, size), dit));
            fit = std::copy(std::prev(dit, size), std::end(data), std::begin(frame));
        }

        // add tail information
        if (std::distance(fit, std::end(frame)) < std::ptrdiff_t{sizeof(size)})
            encoder_.push(frame);
        helpers::copy(Size(encoder_.size() + 1), std::rbegin(frame));
        encoder_.push(frame);

        return encoder_.size() + redundancy;
    }

    /// pop
    /// @return coded vector
    Vector pop() { return std::move(encoder_.pop(1).front()); }

  protected:
    encoder<Vector> encoder_;
};


/// ===============================================================================================
/// ostream
/// @brief
/// ===============================================================================================
template <typename Vector = std::vector<uint8_t>, typename Size = uint32_t>
class ostream {
    static constexpr int DEFAULT_CAPACITY = 100;

  public:
    /// constructor
    /// @param capacity
    /// @param token
    ostream(
      size_t capacity            = DEFAULT_CAPACITY,
      token::shared::Stamp token = token::get(token::Type::FULL))
      : decoder_(capacity, token) {}

    /// push
    /// @param frame coded
    /// @return decode count
    size_t push(Vector frame) {
        // digest data
        decoder_.push(std::move(frame));

        // check tail
        auto count = Size{0};
        helpers::copy(std::rbegin(decoder_.back()), count);
        if (count != decoder_.size())
            return 0;

        // check front
        auto size = Size{0};
        helpers::copy(std::begin(decoder_.front()), size);
        auto max = (decoder_.size() * decoder_.front().size()) - (2 * sizeof(Size));
        if (size > max)
            return 0;
        auto min = max - decoder_.back().size();
        if (size < min)
            return 0;

        return count;
    }

    /// get
    /// @return decoder frame
    Vector get() {
        auto out = Vector();

        if (auto dit = std::begin(decoder_); dit != std::end(decoder_)) {
            out.reserve(decoder_.size() * dit->size());

            auto size = Size(0);
            helpers::copy(std::begin(*dit), size);
            std::copy(helpers::copy(std::begin(*dit), size), std::end(*dit), std::back_inserter(out));
            for (++dit; dit != end(decoder_); ++dit)
                std::copy(std::begin(*dit), std::end(*dit), std::back_inserter(out));
            out.resize(size);

            decoder_.clear();
        }
        return out;
    }

  protected:
    decoder<Vector> decoder_;
};

} // namespace share::codec