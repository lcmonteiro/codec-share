/// ===============================================================================================
/// @file      : solve.hpp                                                 |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \ 
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

#include <cstddef>
#include <type_traits>

#include "gf8.hpp"

namespace detail {

namespace {
    template <typename Matrix>
    static inline void elimination(Matrix& coef, Matrix& data, size_t index) {
        for (uint32_t i = index + 1; i < data.size(); ++i) {
            if (coef[i][index] == 0) {
                continue;
            }
            // compute factor
            auto factor = gf8::div(coef[index][index], coef[i][index]);
            // verify factor
            if (factor == 1) {
                gf8::sum(coef[i], coef[index], index);
                gf8::sum(data[i], data[index]);
                continue;
            }
            // multiply and sum
            gf8::sum(gf8::mul(coef[i], factor, index), coef[index], index);
            gf8::sum(gf8::mul(data[i], factor), data[index]);
        }
    }

    template <typename Matrix>
    static inline bool prepare(Matrix& coef, Matrix& data, size_t index) {
        if (coef[index][index]) {
            return true;
        }
        for (int i = index + 1, ii = int(data.size()); i < ii; ++i) {
            if (coef[i][index]) {
                std::swap(coef[index], coef[i]);
                std::swap(data[index], data[i]);
                return true;
            }
        }
        return false;
    }

    template <typename Matrix>
    static inline void reverse_elimination(Matrix& coef, Matrix& data, size_t index) {
        for (int i = 0; i < index; ++i) {
            if (coef[i][index] == 0) {
                continue;
            }
            // compute factor
            auto factor = gf8::div(coef[i][index], coef[index][index]);
            // verify factor
            if (factor == 1) {
                gf8::sum(coef[i], coef[index], index);
                gf8::sum(data[i], data[index]);
                continue;
            }
            // multiply and sum (= merge)
            gf8::sum(coef[i], gf8::mul(coef[index], factor, index), index);
            gf8::sum(data[i], gf8::mul(data[index], factor));
        }
    }

    template <typename Matrix>
    static inline void unification(Matrix& coef, Matrix& data, int index) {
        auto factor = gf8::div(std::decay_t<decltype(coef[index][index])>(1), coef[index][index]);
        if (factor == 0) {
            return;
        }
        if (factor == 1) {
            return;
        }
        gf8::mul(coef[index], factor, index);
        gf8::mul(data[index], factor);
    }

    template <typename Vector, typename Matrix>
    static inline void organize(Vector& field, Matrix& coef, Matrix& data) {
        if (field.empty()) {
            return;
        }
        for (auto i = size_t(0), ii = field.size() - 1; i < ii;) {
            if (field[i] == 1) {
                i++;
                if (field[ii] != 1) {
                    ii--;
                }
            } else {
                if (field[ii] == 1) {
                    std::swap(field[i], field[ii]);
                    std::swap(coef[i], coef[ii]);
                    std::swap(data[i], data[ii]);
                    i++;
                }
                ii--;
            }
        }
    }
} // namespace

/// solve
/// @brief 
/// solve gf8 combination system
template <typename Vector, typename Matrix>
static size_t solve(size_t size, Vector& field, Matrix& coef, Matrix& data) {
    size_t n = 0;
    // organize data
    organize(field, coef, data);
    // forward elemination
    for (; n < size && n < data.size(); ++n) {
        if (!prepare(coef, data, n))
            break;
        elimination(coef, data, n);
    }
    // backward elemination
    for (size_t i = 0; i < n; ++i)
        reverse_elimination(coef, data, i);
    // diagonal unification
    for (size_t i = 0; i < n; ++i)
        unification(coef, data, i);
    return n;
}
} // namespace detail
