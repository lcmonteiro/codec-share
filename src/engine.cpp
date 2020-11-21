/// @file engine.cpp
/// @author Luis Monteiro
/// @date November 30, 2015, 5:48 PM

#include "engine.hpp"

#include <iostream>
#include <set>

/// Implementation
namespace Codec {
namespace Engine {
    using namespace Container;
    using namespace Token;

    namespace System {
        static inline size_t
        Combine(const Frames& data, uint32_t seed, uint8_t field, uint8_t sparsity, Frame& out);
    } // namespace System

    Frames Encode(const Stamp& token, const Frames& data, size_t size) {
        // coded container
        Frames code;
        // random generetor
        Random rand;
        // sizes
        auto data_length = data.length();
        auto code_length = data_length + HeaderSize();
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
                field    = token[uint8_t(seed)].first;
                sparsity = token[uint8_t(seed)].second;
            } while (System::Combine(data, seed, field, sparsity, comb) == 0);
            // add seed
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

    namespace System {
        static size_t Solve(size_t size, Frame& field, Frames& coef, Frames& data);
    } // namespace System
    size_t Decode(
        const Stamp& token,
        Frames code,
        size_t size,
        Frames& data,
        Frames& coefs,
        Frame& fields) {
        // prepare
        for (auto& frame : code) {
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
            auto field     = uint8_t{token[uint8_t(seed)].first};
            auto sparsity  = uint8_t{token[uint8_t(seed)].second};
            auto generator = Generator{seed};
            // gerenate coefficients
            auto coef = Frame{size + sizeof(int), size, 0};
            for (auto& val : coef) {
                auto factor = uint8_t(generator());
                if (factor > sparsity)
                    continue;
                val = (factor & field);
            }
            // update
            data.push(std::move(frame));
            coefs.push(std::move(coef));
            fields.push(field);
        }
        return System::Solve(size, fields, coefs, data);
    }

    namespace GF8 {
        static constexpr int INT_MASK = ~int(sizeof(int) - 1);
        static const int V2P[]        = {
            512, 0,   1,   25,  2,   50,  26,  198, 3,   223, 51,  238, 27,  104, 199, 75,
            4,   100, 224, 14,  52,  141, 239, 129, 28,  193, 105, 248, 200, 8,   76,  113,
            5,   138, 101, 47,  225, 36,  15,  33,  53,  147, 142, 218, 240, 18,  130, 69,
            29,  181, 194, 125, 106, 39,  249, 185, 201, 154, 9,   120, 77,  228, 114, 166,
            6,   191, 139, 98,  102, 221, 48,  253, 226, 152, 37,  179, 16,  145, 34,  136,
            54,  208, 148, 206, 143, 150, 219, 189, 241, 210, 19,  92,  131, 56,  70,  64,
            30,  66,  182, 163, 195, 72,  126, 110, 107, 58,  40,  84,  250, 133, 186, 61,
            202, 94,  155, 159, 10,  21,  121, 43,  78,  212, 229, 172, 115, 243, 167, 87,
            7,   112, 192, 247, 140, 128, 99,  13,  103, 74,  222, 237, 49,  197, 254, 24,
            227, 165, 153, 119, 38,  184, 180, 124, 17,  68,  146, 217, 35,  32,  137, 46,
            55,  63,  209, 91,  149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190, 97,
            242, 86,  211, 171, 20,  42,  93,  158, 132, 60,  57,  83,  71,  109, 65,  162,
            31,  45,  67,  216, 183, 123, 164, 118, 196, 23,  73,  236, 127, 12,  111, 246,
            108, 161, 59,  82,  41,  157, 85,  170, 251, 96,  134, 177, 187, 204, 62,  90,
            203, 89,  95,  176, 156, 169, 160, 81,  11,  245, 22,  235, 122, 117, 44,  215,
            79,  174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168, 80,  88,  175};

        static const int P2V[] = {
            1,   2,   4,   8,   16,  32,  64,  128, 29,  58,  116, 232, 205, 135, 19,  38,  76,
            152, 45,  90,  180, 117, 234, 201, 143, 3,   6,   12,  24,  48,  96,  192, 157, 39,
            78,  156, 37,  74,  148, 53,  106, 212, 181, 119, 238, 193, 159, 35,  70,  140, 5,
            10,  20,  40,  80,  160, 93,  186, 105, 210, 185, 111, 222, 161, 95,  190, 97,  194,
            153, 47,  94,  188, 101, 202, 137, 15,  30,  60,  120, 240, 253, 231, 211, 187, 107,
            214, 177, 127, 254, 225, 223, 163, 91,  182, 113, 226, 217, 175, 67,  134, 17,  34,
            68,  136, 13,  26,  52,  104, 208, 189, 103, 206, 129, 31,  62,  124, 248, 237, 199,
            147, 59,  118, 236, 197, 151, 51,  102, 204, 133, 23,  46,  92,  184, 109, 218, 169,
            79,  158, 33,  66,  132, 21,  42,  84,  168, 77,  154, 41,  82,  164, 85,  170, 73,
            146, 57,  114, 228, 213, 183, 115, 230, 209, 191, 99,  198, 145, 63,  126, 252, 229,
            215, 179, 123, 246, 241, 255, 227, 219, 171, 75,  150, 49,  98,  196, 149, 55,  110,
            220, 165, 87,  174, 65,  130, 25,  50,  100, 200, 141, 7,   14,  28,  56,  112, 224,
            221, 167, 83,  166, 81,  162, 89,  178, 121, 242, 249, 239, 195, 155, 43,  86,  172,
            69,  138, 9,   18,  36,  72,  144, 61,  122, 244, 245, 247, 243, 251, 235, 203, 139,
            11,  22,  44,  88,  176, 125, 250, 233, 207, 131, 27,  54,  108, 216, 173, 71,  142,
            1,   2,   4,   8,   16,  32,  64,  128, 29,  58,  116, 232, 205, 135, 19,  38,  76,
            152, 45,  90,  180, 117, 234, 201, 143, 3,   6,   12,  24,  48,  96,  192, 157, 39,
            78,  156, 37,  74,  148, 53,  106, 212, 181, 119, 238, 193, 159, 35,  70,  140, 5,
            10,  20,  40,  80,  160, 93,  186, 105, 210, 185, 111, 222, 161, 95,  190, 97,  194,
            153, 47,  94,  188, 101, 202, 137, 15,  30,  60,  120, 240, 253, 231, 211, 187, 107,
            214, 177, 127, 254, 225, 223, 163, 91,  182, 113, 226, 217, 175, 67,  134, 17,  34,
            68,  136, 13,  26,  52,  104, 208, 189, 103, 206, 129, 31,  62,  124, 248, 237, 199,
            147, 59,  118, 236, 197, 151, 51,  102, 204, 133, 23,  46,  92,  184, 109, 218, 169,
            79,  158, 33,  66,  132, 21,  42,  84,  168, 77,  154, 41,  82,  164, 85,  170, 73,
            146, 57,  114, 228, 213, 183, 115, 230, 209, 191, 99,  198, 145, 63,  126, 252, 229,
            215, 179, 123, 246, 241, 255, 227, 219, 171, 75,  150, 49,  98,  196, 149, 55,  110,
            220, 165, 87,  174, 65,  130, 25,  50,  100, 200, 141, 7,   14,  28,  56,  112, 224,
            221, 167, 83,  166, 81,  162, 89,  178, 121, 242, 249, 239, 195, 155, 43,  86,  172,
            69,  138, 9,   18,  36,  72,  144, 61,  122, 244, 245, 247, 243, 251, 235, 203, 139,
            11,  22,  44,  88,  176, 125, 250, 233, 207, 131, 27,  54,  108, 216, 173, 71,  142,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

        static inline int Div(int a, int b) {
            if (a == 0) {
                return 0;
            }
            int x = V2P[a] - V2P[b];
            if (x < 0) {
                x += 255;
            }
            return P2V[x];
        }

        static inline int Mul(int a, int b) {
            if (a == 0 || b == 0) {
                return 0;
            }
            return P2V[V2P[a] + V2P[b]];
        }

        static inline Frame& Mul(Frame& b, int m) {
            if (m == 0) {
                fill(b.begin(), b.end(), 0);
                return b;
            }
            if (m == 1) {
                return b;
            }
            auto M = V2P[m];
            for (auto p0 = b.data(), pe = p0 + b.size(); p0 < pe; ++p0) {
                *p0 = P2V[V2P[*p0] + M];
            }
            return b;
        }

        static inline Frame& Mul(Frame& b, int m, int i) {
            if (m == 0) {
                fill(b.begin() + i, b.end(), 0);
                return b;
            }
            if (m == 1) {
                return b;
            }
            auto M = V2P[m];
            for (auto p0 = b.data() + i, pe = b.data() + b.size(); p0 < pe; ++p0) {
                *p0 = P2V[V2P[*p0] + M];
            }
            return b;
        }

        static inline Frame& Sum(Frame& a, Frame& b) {
            int* p0 = reinterpret_cast<int*>(a.data());
            int* p1 = reinterpret_cast<int*>(b.data());
            int* pe = reinterpret_cast<int*>(a.data() + a.size());
            for (; p0 < pe; ++p0, ++p1) {
                *p0 ^= *p1;
            }
            return a;
        }

        static inline Frame& Sum(Frame& a, Frame& b, int i) {
            int* p0 = reinterpret_cast<int*>(a.data() + (i & INT_MASK));
            int* p1 = reinterpret_cast<int*>(b.data() + (i & INT_MASK));
            int* pe = reinterpret_cast<int*>(a.data() + a.size());
            for (; p0 < pe; ++p0, ++p1) {
                *p0 ^= *p1;
            }
            return a;
        }
    } // namespace GF8

    namespace System {

        // Build a combination
        static inline size_t
        Combine(const Frames& data, uint32_t seed, uint8_t field, uint8_t sparsity, Frame& out) {
            // combine loop
            auto aux     = Frame{out.capacity()};
            auto gen     = Generator{seed};
            auto factor  = uint8_t{0};
            auto counter = size_t{0};
            for (auto& frame : data) {
                factor = gen();
                if (factor > sparsity)
                    continue;
                factor &= field;
                if (factor == 0)
                    continue;
                // copy for calculation process
                aux.assign(frame.begin(), frame.end());
                // calculation process (Y += Xn * Cn)
                GF8::Sum(out, GF8::Mul(aux, factor));
                // track number of merges
                ++counter;
            }
            return counter;
        }

        /// Solve a combination system
        static inline void Elimination(Frames& coef, Frames& data, int index) {
            for (uint32_t i = index + 1; i < data.size(); ++i) {
                if (coef[i][index] == 0) {
                    continue;
                }
                // compute factor
                auto factor = GF8::Div(coef[index][index], coef[i][index]);
                // verify factor
                if (factor == 1) {
                    GF8::Sum(coef[i], coef[index], index);
                    GF8::Sum(data[i], data[index]);
                    continue;
                }
                // multiply and sum
                GF8::Sum(GF8::Mul(coef[i], factor, index), coef[index], index);
                GF8::Sum(GF8::Mul(data[i], factor), data[index]);
            }
        }
        static inline bool Prepare(Frames& coef, Frames& data, int index) {
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
        static inline void ReverseElimination(Frames& coef, Frames& data, int index) {
            for (int i = 0; i < index; ++i) {
                if (coef[i][index] == 0) {
                    continue;
                }
                // compute factor
                auto factor = GF8::Div(coef[i][index], coef[index][index]);
                // verify factor
                if (factor == 1) {
                    GF8::Sum(coef[i], coef[index], index);
                    GF8::Sum(data[i], data[index]);
                    continue;
                }
                // multiply and sum (= merge)
                GF8::Sum(coef[i], GF8::Mul(coef[index], factor, index), index);
                GF8::Sum(data[i], GF8::Mul(data[index], factor));
            }
        }
        static inline void Unification(Frames& coef, Frames& data, int index) {
            auto factor = GF8::Div(1, coef[index][index]);
            if (factor == 0) {
                return;
            }
            if (factor == 1) {
                return;
            }
            GF8::Mul(coef[index], factor, index);
            GF8::Mul(data[index], factor);
        }
        static inline void Organize(Frame& field, Frames& coef, Frames& data) {
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
        static size_t Solve(size_t size, Frame& field, Frames& coef, Frames& data) {
            size_t n = 0;
            // organize data
            Organize(field, coef, data);
            // forward elemination
            for (; n < size && n < data.size(); ++n) {
                if (!Prepare(coef, data, n)) {
                    break;
                }
                Elimination(coef, data, n);
            }
            // backward elemination
            for (size_t i = 0; i < n; ++i) {
                ReverseElimination(coef, data, i);
            }
            // diagonal unification
            for (size_t i = 0; i < n; ++i) {
                Unification(coef, data, i);
            }
            return n;
        }
    } // namespace System
} // namespace Engine
} // namespace Codec