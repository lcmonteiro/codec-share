/// ===============================================================================================
/// @file      : stream.hpp                                                |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \ 
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================
#pragma once

#include "decoder.hpp"
#include "encoder.hpp"

namespace codec {

/// ===============================================================================================
/// istream
/// @brief
/// ===============================================================================================
template <typename Vector, typename Size = uint32_t>
class istream {
  public:
    using Container = container<Vector>;
    using Encoder   = encoder<Vector>;

    /// set
    /// @param data
    /// @param framesize
    /// @param redundancy
    Size set(const Vector& data, Size framesize, Size redundancy = 0) {
        // helpers
        Size size  = min(framesize - encoder_.HEADER_SIZE, sizeof(Size));
        auto frame = Vector(size, size, 0);

        // add head information
        auto fit = copy(size, begin(frame));
        auto dit = next(begin(data), min(data.size(), size - sizeof(size)));

        // split data and add to encoder
        if (copy(begin(data), dit, fit) == end(frame)) {
            encoder_.push(frame);
            for (advance(dit, size); dit < end(data); advance(dit, size))
                encoder_.push(Vector(prev(dit, size), dit));
            fit = copy(prev(dit, size), end(data), begin(frame));
        }

        // add tail information
        if (distance(fit, end(frame)) >= sizeof(size)) {
            copy(Size(encoder_.size() + 1), rbegin(frame));
            encoder_.push(frame);
        } else {
            encoder_.push(frame);
            copy(Size(encoder_.size() + 2), rbegin(frame));
            encoder_.push(frame);
        }
        return encoder_.size() + redundancy;
    }

    /// pop
    /// @return coded vector
    Vector pop();

  protected:
    Encoder encoder_;
};


/// ===============================================================================================
/// ostream
/// @brief
/// ===============================================================================================
template <typename Vector>
class ostream {
    static constexpr int DEFAULT_CAPACITY = 100;

  public:
    using Container = container<Vector>;
    using Decoder   = decoder<Vector>;

    /// constructor
    /// @param capacity
    /// @param token
    ostream(size_t capacity = DEFAULT_CAPACITY) : decoder_(capacity) {}

    /// push
    /// @param frame coded
    /// @return decode count
    size_t push(Vector frame);

    /// get
    /// @return decoder frame
    Container get();

  protected:
    Decoder decoder_;
};

} // namespace codec
