/// ===============================================================================================
/// @file      : copy.hpp                                               |
/// @copyright : 2019 LCMonteiro                                     __|   __ \    _` |   __|  _ \. 
///                                                                 \__ \  | | |  (   |  |     __/
/// @author    : Luis Monteiro                                      ____/ _| |_| \__,_| _|   \___|
/// ===============================================================================================

#pragma once

namespace share::codec::helpers {
/// copy
/// @brief serialize a interger to a iterator
/// @param num
/// @param it
template <typename Iterator>
std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>, Iterator>
copy(uint32_t num, Iterator it) {
    *it = uint8_t(num), ++it, num <<= 8;
    *it = uint8_t(num), ++it, num <<= 8;
    *it = uint8_t(num), ++it, num <<= 8;
    *it = uint8_t(num), ++it;
    return it;
}
template <typename Iterator>
std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>, Iterator>
copy(uint16_t num, Iterator it) {
    *it = uint8_t(num), ++it, num <<= 8;
    *it = uint8_t(num), ++it;
    return it;
}
template <typename Iterator>
std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>, Iterator>
copy(uint8_t num, Iterator it) {
    *it = uint8_t(num), ++it;
    return it;
}

/// copy
/// @brief unserialize a interger from a iterator
/// @param it
/// @param num
template <typename Iterator>
std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>, Iterator>
copy(Iterator it, uint32_t& num) {
    num = uint32_t(*it), ++it;
    num |= uint32_t(*it) << 0x08, ++it;
    num |= uint32_t(*it) << 0x10, ++it;
    num |= uint32_t(*it) << 0x18, ++it;
    return it;
}
template <typename Iterator>
std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>, Iterator>
copy(Iterator it, uint16_t& num) {
    num = uint32_t(*it), ++it;
    num |= uint32_t(*it) << 0x08, ++it;
    return it;
}
template <typename Iterator>
std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>, Iterator>
copy(Iterator it, uint8_t& num) {
    num = uint32_t(*it), ++it;
    return it;
}

} // namespace share::codec::helpers
