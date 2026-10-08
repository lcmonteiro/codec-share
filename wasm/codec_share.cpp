/// ===============================================================================================
/// @file      : codec_share.cpp                                           |
/// @copyright : 2026 LCMonteiro                                     __|   __ \    _` |   __|  _ \.
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================
/// WebAssembly entry points
///
///   A flat C interface over the codec, built as a WASI reactor so any wasm runtime can load it
///   (see wasm/codec_share.py for the python binding).
///
///   seal   data -> [magic|length|checksum|data|padding] split in k frames, coded in n frames
///   open   any k independent coded frames + the same stamp -> data
///
///   Without the stamp used to seal, the frames do not decode (detected by the checksum).
/// ===============================================================================================

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "decoder.hpp"
#include "encoder.hpp"
#include "helpers/copy.hpp"

#define EXPORT(name) extern "C" __attribute__((export_name(#name)))

namespace {
using Vector    = std::vector<uint8_t>;
using Container = share::codec::container<Vector>;

/// errors
enum : int32_t {
    ERROR_ARGUMENT = -1, // invalid arguments
    ERROR_SHARES   = -2, // not enough independent frames
    ERROR_STAMP    = -3, // frames do not open with this stamp (or are corrupted)
    ERROR_BUFFER   = -4, // output buffer too small
};

/// layout
constexpr uint32_t MAGIC       = 0x31305343; // "CS01"
constexpr uint32_t HEADER_SIZE = 3 * sizeof(uint32_t);
constexpr uint32_t SEED_SIZE   = sizeof(uint32_t);
constexpr uint32_t STAMP_SIZE  = 256 * 2;
constexpr uint32_t MAX_FRAMES  = 255;
constexpr uint32_t MAX_SPLIT   = 16;

/// fnv-1a checksum
uint32_t checksum(const uint8_t* data, uint32_t size) {
    auto hash = uint32_t{0x811c9dc5};
    for (auto end = data + size; data != end; ++data)
        hash = (hash ^ *data) * 0x01000193;
    return hash;
}

/// data frame size, a multiple of the word size (the gf8 operations work in words)
uint32_t data_frame_size(uint32_t size, uint32_t split) {
    auto frame = (uint64_t{HEADER_SIZE} + size + split - 1) / split;
    return uint32_t((frame + sizeof(int) - 1) & ~uint64_t{sizeof(int) - 1});
}

/// stamp from its raw form: 256 pairs of (field, sparsity)
share::codec::token::shared::Stamp load_stamp(const uint8_t* raw) {
    auto stamp  = share::codec::token::Stamp(256);
    auto usable = 0;
    for (auto& density : stamp) {
        density.first  = *raw++;
        density.second = *raw++;
        // densities able to merge most of the frames, required to seal in a sane time
        usable += (__builtin_popcount(density.first) >= 3 && density.second >= 127);
    }
    if (usable < 32)
        return nullptr;
    return std::make_shared<const share::codec::token::Stamp>(std::move(stamp));
}
} // namespace

/// memory
EXPORT(cs_alloc) void* cs_alloc(uint32_t size) { return std::malloc(size ? size : 1); }
EXPORT(cs_free) void cs_free(void* ptr) { std::free(ptr); }

/// stamp size
EXPORT(cs_stamp_size) uint32_t cs_stamp_size() { return STAMP_SIZE; }

/// generate a stamp
/// @param type  token type (0 sparse, 1 stream, 2 message, 3 full)
/// @param seed
/// @param out   STAMP_SIZE bytes
/// @return STAMP_SIZE or error
EXPORT(cs_stamp_generate) int32_t cs_stamp_generate(uint32_t type, uint64_t seed, uint8_t* out) {
    if (type > uint32_t(share::codec::token::Type::FULL) || !out)
        return ERROR_ARGUMENT;
    auto stamp = share::codec::token::generate(share::codec::token::Type(type), seed);
    for (auto& density : *stamp) {
        *out++ = density.first;
        *out++ = density.second;
    }
    return STAMP_SIZE;
}

/// coded frame size
/// @param size  data size
/// @param split number of frames needed to open
/// @return frame size or error
EXPORT(cs_frame_size) int32_t cs_frame_size(uint32_t size, uint32_t split) {
    if (split == 0 || split > MAX_SPLIT || size > (1u << 30))
        return ERROR_ARGUMENT;
    return int32_t(data_frame_size(size, split) + SEED_SIZE);
}

/// seal data in coded frames
/// @param stamp STAMP_SIZE bytes
/// @param data
/// @param size
/// @param split number of frames needed to open (k)
/// @param count number of coded frames (n >= k)
/// @param out   count * cs_frame_size(size, split) bytes
/// @param cap   out capacity
/// @return frame size or error
EXPORT(cs_seal)
int32_t cs_seal(
  const uint8_t* stamp, const uint8_t* data, uint32_t size, uint32_t split, uint32_t count,
  uint8_t* out, uint32_t cap) {
    auto frame_size = cs_frame_size(size, split);
    if (frame_size < 0 || !stamp || (!data && size) || count < split || count > MAX_FRAMES)
        return ERROR_ARGUMENT;
    if (uint64_t{cap} < uint64_t{count} * uint32_t(frame_size) || !out)
        return ERROR_BUFFER;
    auto token = load_stamp(stamp);
    if (!token)
        return ERROR_STAMP;

    // header + data + padding
    auto length = data_frame_size(size, split);
    auto buffer = Vector(size_t{length} * split, 0);
    auto it     = buffer.begin();
    it          = share::codec::helpers::copy(MAGIC, it);
    it          = share::codec::helpers::copy(size, it);
    it          = share::codec::helpers::copy(checksum(data, size), it);
    std::copy(data, data + size, it);

    // split and code, every coded frame merges all data frames
    auto encoder = share::codec::encoder<Vector>(split, token);
    for (auto pos = buffer.begin(); pos != buffer.end(); pos += length)
        encoder.push(Vector(pos, pos + length));
    for (auto& frame : encoder.pop(count, split))
        out = std::copy(frame.begin(), frame.end(), out);
    return frame_size;
}

/// open coded frames
/// @param stamp STAMP_SIZE bytes
/// @param data  count * frame bytes
/// @param frame coded frame size
/// @param count number of coded frames
/// @param split number of frames needed to open (k)
/// @param out
/// @param cap   out capacity
/// @return data size or error
EXPORT(cs_open)
int32_t cs_open(
  const uint8_t* stamp, const uint8_t* data, uint32_t frame, uint32_t count, uint32_t split,
  uint8_t* out, uint32_t cap) {
    if (!stamp || !data || split == 0 || split > MAX_SPLIT || count == 0 || count > MAX_FRAMES)
        return ERROR_ARGUMENT;
    if (frame <= SEED_SIZE || (frame - SEED_SIZE) % sizeof(int))
        return ERROR_ARGUMENT;
    if (count < split)
        return ERROR_SHARES;
    auto token = load_stamp(stamp);
    if (!token)
        return ERROR_STAMP;

    // decode
    auto coded = Container();
    for (auto end = data + uint64_t{frame} * count; data != end; data += frame)
        coded.push_back(Vector(data, data + frame));
    auto decoder = share::codec::decoder<Vector>(split, token);
    decoder.push(std::move(coded));
    if (decoder.size() < split)
        return ERROR_SHARES;

    // join
    auto buffer = Vector();
    for (auto& part : decoder.pop())
        buffer.insert(buffer.end(), part.begin(), part.end());

    // verify
    auto magic = uint32_t{}, size = uint32_t{}, check = uint32_t{};
    auto it    = share::codec::helpers::copy(buffer.begin(), magic);
    it         = share::codec::helpers::copy(it, size);
    it         = share::codec::helpers::copy(it, check);
    if (magic != MAGIC || size > buffer.size() - HEADER_SIZE || checksum(buffer.data() + HEADER_SIZE, size) != check)
        return ERROR_STAMP;
    if (cap < size || (!out && size))
        return ERROR_BUFFER;
    std::copy(it, it + size, out);
    return int32_t(size);
}
