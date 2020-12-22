/// @file:   stamp.cpp
/// @author: Luis Monteiro
/// @date:   Created on November 30, 2015, 5:48 PM

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <set>

#include "stream.hpp"

namespace Codec {

using namespace Container;
using namespace std;

/**
 * ------------------------------------------------------------------------------------------------
 * Encode interface
 * ------------------------------------------------------------------------------------------------
 **/
#define SET_UINT32(VAL)              \
    do {                             \
        uint32_t aux = (VAL);        \
        *rit         = uint8_t(aux); \
        ++rit;                       \
        aux >>= 8;                   \
        *rit = uint8_t(aux);         \
        ++rit;                       \
        aux >>= 8;                   \
        *rit = uint8_t(aux);         \
        ++rit;                       \
        aux >>= 8;                   \
        *rit = uint8_t(aux);         \
        ++rit;                       \
    } while (0)


/// ===============================================================================================
/// helpers
/// @brief
/// ===============================================================================================
template <typename Number, typename Iterator>
Iterator copy(Number num, Iterator it) {
    using Value = typename Iterator::value_type;

    constexpr auto n = std::ceil(sizeof(Number) / sizeof(Value));
    constexpr auto s = sizeof(Value) * 8;
    for (auto i = 0; i < n; ++i, num >>= s, ++it)
        *it = Value(num);
    return it;
}


/// ===============================================================================================
/// IShareStream
/// @brief
/// [ hhhhhhhh..xxxx..xxxx| xxxx..xxxx | ... | xxxx..xxxx | xxxx..xxxx..hhhhhhhh]
/// ===============================================================================================
size_t IShareStream::set(const Frame& data, size_t framesize, size_t redundancy) {
    // helpers
    auto size  = uint32_t(min(framesize - encoder_.HEADER_SIZE, sizeof(uint32_t)));
    auto frame = Frame(size, size, 0);

    // add head information
    auto fit = copy(size, begin(frame));
    auto dit = next(begin(data), min(data.size(), size - sizeof(size)));

    // split data and add to encoder
    if (copy(begin(data), dit, fit) == end(frame)) {
        encoder_.push(frame);
        //for (advance(dit, size); dit < end(data); advance(dit, size))
            //encoder_.push(Frame(prev(dit, size), dit));
        fit = copy(prev(dit, size), end(data), begin(frame));
    }

    // add tail information
    auto count = uint32_t(encoder_.size());
    if (distance(fit, end(frame)) >= sizeof(size)) {
        copy(count + 1, rbegin(frame));
        encoder_.push(frame);
    } else {
        encoder_.push(frame);
        copy(count + 1, rbegin(frame));
        encoder_.push(frame);
    }
    return count + redundancy;
}

Frame IShareStream::pop() {
    return move(encoder_.pop(1).front());
}

/**
 * ------------------------------------------------------------------------------------------------
 * Decode interface
 * ------------------------------------------------------------------------------------------------
 **/
#define GET_UINT32(VAR)              \
    do {                             \
        VAR = uint32_t(*rit);        \
        ++rit;                       \
        VAR |= uint32_t(*rit) << 8;  \
        ++rit;                       \
        VAR |= uint32_t(*rit) << 16; \
        ++rit;                       \
        VAR |= uint32_t(*rit) << 24; \
        ++rit;                       \
    } while (0)

/// ===============================================================================================
/// OShareStream
/// @brief
/// [ hhhhhhhh..xxxx..xxxx| xxxx..xxxx | ... | xxxx..xxxx | xxxx..xxxx..hhhhhhhh]
/// ===============================================================================================
size_t OShareStream::push(Frame frame) {
    // digest data
    decoder_.push(move(frame));

    // check tail
    auto count = 0;
    // copy(decoder_.back().rbegin(), count);
    if (count != decoder_.size())
        return 0;

    // check front
    auto size = 0;
    // copy(decoder_.front().begin(), size);
    if (size >= decoder_.size() * decoder_.front().size())
        return 0;
    if (size < (decoder_.size() - 2) * decoder_.front().size())
        return 0;
    return count;
}

Frame OShareStream::get() {
    if (auto dit = begin(decoder_); dit != end(decoder_)) {
        auto size = 0;
        // dit = copy(dit, size);
        auto out = Frame(size);
        //std::copy(begin(*dit), end(*dit), back_inserter(out));
        
        //for (++dit; dit != end(decoder_); ++dit)
        //    std::copy(begin(*dit), end(*dit), back_inserter(out));

        decoder_.clear();
        out.resize(size);
        return out;
    }
    return {};
}

} // namespace Codec
