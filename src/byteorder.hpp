// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_BYTEORDER_HPP
#define LIBMBOXID_BYTEORDER_HPP

#include <cstdint>
#include <concepts>

namespace mboxid {

template <typename T>
requires std::is_integral_v<T> || std::is_enum_v<T> std::size_t fetch8(
        T& dst, const std::uint8_t* buf) {
    std::uint8_t v;
    static_assert(sizeof(T) >= sizeof(v));

    v = *buf;
    dst = static_cast<T>(v);
    return sizeof(v);
}

template <typename T>
requires std::is_integral_v<T> || std::is_enum_v<T> std::size_t fetch16_be(
        T& dst, const std::uint8_t* buf) {
    std::uint16_t v;
    static_assert(sizeof(T) >= sizeof(v));

    v = static_cast<std::uint16_t>(buf[0] << 8) | buf[1];
    dst = static_cast<T>(v);
    return sizeof(v);
}

template <typename T>
requires std::is_integral_v<T> || std::is_enum_v<T> std::size_t store8(
        std::uint8_t* buf, const T val) {
    auto v = static_cast<std::uint8_t>(val);
    *buf = v;
    return sizeof(v);
}

template <typename T>
requires std::is_integral_v<T> || std::is_enum_v<T> std::size_t store16_be(
        std::uint8_t* buf, const T val) {
    auto v = static_cast<std::uint16_t>(val);
    buf[0] = (v >> 8) & 0xffU;
    buf[1] = v & 0xffU;
    return sizeof(v);
}

} // namespace mboxid

#endif // LIBMBOXID_BYTEORDER_HPP
