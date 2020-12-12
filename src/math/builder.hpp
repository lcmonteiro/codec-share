/// @file  : solver.hpp
/// @author: Luis Monteiro
/// @date  : Created on September 29, 2015, 5:48 PM

#pragma once

#include <cstddef>
#include <cstdint>

#include "gf8.hpp"

namespace builder {

template <typename Generator, typename Vector, typename Matrix>
static inline size_t
combine(const Matrix& input, uint32_t seed, uint8_t field, uint8_t sparsity, Vector& output) {
    // combine loop
    auto aux     = Vector{output.capacity()};
    auto gen     = Generator{seed};
    auto factor  = uint8_t{0};
    auto counter = size_t{0};
    for (auto& frame : input) {
        factor = gen();
        if (factor > sparsity)
            continue;
        factor &= field;
        if (factor == 0)
            continue;
        // copy for calculation process
        aux.assign(frame.begin(), frame.end());
        // calculation process (Y += Xn * Cn)
        gf8::sum(output, gf8::mul(aux, factor));
        // track number of merges
        ++counter;
    }
    return counter;
}
} // namespace builder