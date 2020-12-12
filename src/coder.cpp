/// @file engine.cpp
/// @author Luis Monteiro
/// @date November 30, 2015, 5:48 PM
#include "decoder.hpp"
#include "encoder.hpp"

#include <random>

#include "math/builder.hpp"
#include "math/solver.hpp"


/// Implementation
namespace Codec {

using namespace Container;

/// definitions
using Random    = std::random_device;
using Generator = std::minstd_rand0;

/// push
/// @param size
/// @return data
Frames Encoder::pop(size_t size) {
    // coded container
    Frames code;
    // random generetor
    Random rand;
    // sizes
    auto data_length = data_.length();
    auto code_length = data_length + HEADER_SIZE;
    // encode loop
    for (unsigned int i = 0; i < size; i++) {
        // seed, field and sparsity
        auto seed     = uint32_t(0);
        auto field    = uint8_t(0);
        auto sparsity = uint8_t(0);
        // create combination
        Frame comb{code_length + sizeof(int), data_length, 0};
        do {
            seed     = rand();
            field    = (*token_)[uint8_t(seed)].first;
            sparsity = (*token_)[uint8_t(seed)].second;
        } while (builder::combine<Generator>(data_, seed, field, sparsity, comb) == 0);
        // insert seed
        comb.push(uint8_t(seed));
        seed >>= 8;
        comb.push(uint8_t(seed));
        seed >>= 8;
        comb.push(uint8_t(seed));
        seed >>= 8;
        comb.push(uint8_t(seed));
        // save combination
        code.push(std::move(comb));
    }
    return code;
}

/// push
/// @param data
Decoder& Decoder::push(Frames data) {
    for (auto& frame : data) {
        // remove seed
        auto seed = uint32_t(frame.back());
        frame.pop_back();
        seed <<= 8;
        seed |= uint32_t(frame.back());
        frame.pop_back();
        seed <<= 8;
        seed |= uint32_t(frame.back());
        frame.pop_back();
        seed <<= 8;
        seed |= uint32_t(frame.back());
        frame.pop_back();
        // properties
        auto field     = uint8_t{(*token_)[uint8_t(seed)].first};
        auto sparsity  = uint8_t{(*token_)[uint8_t(seed)].second};
        auto generator = Generator{seed};
        // gerenate coefficients
        auto coef = Frame{capacity_ + sizeof(int), capacity_, 0};
        for (auto& val : coef) {
            auto factor = uint8_t(generator());
            if (factor > sparsity)
                continue;
            val = (factor & field);
        }
        data_.push(std::move(frame));
        coef_.push(std::move(coef));
        field_.push(field);
    }
    size_ = solver::solve(capacity_, field_, coef_, data_);
    return *this;
}
} // namespace Codec
