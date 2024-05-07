// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"

namespace mboxid {

void parse_mbap_header(std::span<const uint8_t> src, mbap_header& header) {
    expects(src.size() >= mbap_header_size, "incomplete mbap header");

    const uint8_t* p = src.data();

    p += fetch16_be(header.transaction_id, p);
    p += fetch16_be(header.protocol_id, p);
    p += fetch16_be(header.length, p);
    p += fetch8(header.unit_id, p);

    if (header.protocol_id != 0)
        throw mboxid_error(errc::parse_error,
                           "mbap header: protocol identifier invalid");

    if ((header.length < (min_pdu_size + sizeof(header.unit_id))) ||
         header.length > (max_pdu_size + sizeof(header.unit_id)))
        throw mboxid_error(errc::parse_error,
                           "mbap header: length field invalid");
}

size_t serialize_mbap_header(std::span<uint8_t> dst,
                             const mbap_header& header) {
    expects(dst.size() >= mbap_header_size, "buffer too small");

    uint8_t* p = dst.data();
    p += store16_be(p, header.transaction_id);
    p += store16_be(p, header.protocol_id);
    p += store16_be(p, header.length);
    p += store8(p, header.unit_id);
    return p - dst.data();
}

std::size_t parse_bits(std::span<const uint8_t> src,
                       std::vector<bool>& bits, std::size_t cnt)
{
    auto byte_count = bit_to_byte_count(cnt);

    expects(src.size() >= byte_count, "too few bytes");

    bits.resize(cnt);

    for (size_t i = 0; i < byte_count; ++i) {
        unsigned val = src[i];
        for (size_t j = 0; j < bits_per_byte; ++j) {
            size_t ix = bits_per_byte * i + j;
            if (ix >= cnt)
                break;
            bits[ix] = val & (1U << j);
        }
    }

    return byte_count;
}

size_t serialize_bits(std::span<uint8_t> dst,
                             const std::vector<bool>& bits)
{
    auto byte_count = bit_to_byte_count(bits.size());

    expects(dst.size() >= byte_count, "buffer too small");

    for (size_t i = 0; i < byte_count; ++i) {
        uint8_t val = 0;
        for (size_t j = 0; j < bits_per_byte; ++j) {
            size_t ix = bits_per_byte * i + j;
            if (ix >= bits.size())
                break;
            val |= (bits[ix] << j);
        }
        dst[i] = val;
    }

    return byte_count;
}

std::size_t parse_regs(std::span<const uint8_t> src,
                       std::vector<uint16_t>& regs, std::size_t cnt)
{
    auto byte_count = cnt * sizeof(uint16_t);

    expects(src.size() >= byte_count, "too few bytes");

    regs.resize(cnt);
    for (size_t i = 0; i < cnt; ++i)
        fetch16_be(regs[i], &src[i * sizeof(uint16_t)]);

    return (byte_count);
}

std::size_t serialize_regs(std::span<uint8_t> dst,
                           const std::vector<uint16_t>& regs) {
    expects(dst.size() >= (regs.size() * sizeof(uint16_t)), "buffer too small");

    uint8_t* p = dst.data();
    for (auto v : regs)
        p += store16_be(p, v);

    return p - dst.data();
}

} // namespace mboxid
