// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_client.hpp"

namespace mboxid {

constexpr unsigned min_addr = 0U;
constexpr unsigned max_addr = 0xffffU;

static void validate_exact_rsp_length(std::span<const uint8_t> rsp, size_t len)
{
    if (rsp.size() != len)
        throw mboxid_error(errc::parse_error, "response length invalid");
}

void validate_field(bool cond, const char* msg) {
    if (!cond)
        throw (mboxid_error(errc::parse_error, msg));
}

static void inline check_for_exception(std::span<const uint8_t> rsp,
                                       function_code fc)
{
    if (rsp.size() != exception_rsp_size)
        return;

    unsigned msk = static_cast<unsigned>(function_code::exception);
    unsigned fc_rsp = rsp[0];
    unsigned exception_code = rsp[1];

    if (!(fc_rsp & msk))
        return;

    if ((fc_rsp & ~msk) != static_cast<unsigned>(fc))
        throw mboxid_error(errc::parse_error,
                           "modbus exception: function code mismatch");

    auto err = static_cast<errc>(exception_code);
    if (!is_modbus_exception(err))
        throw mboxid_error(errc::parse_error,
                           "modbus exception: invalid exception code");
    else
        throw mboxid_error(err);
}

size_t serialize_read_bits_request(std::span<uint8_t> dst, function_code fc,
                                   unsigned addr, size_t cnt)
{
    validate_argument(addr, min_addr, max_addr, "read_bits: addr");
    validate_argument(cnt, min_read_bits, max_read_bits, "read_bits: cnt");

    expects(dst.size() >= read_bits_req_size, "buffer too small");

    uint8_t* p = dst.data();

    p += store8(p, fc);
    p += store16_be(p, addr);
    p += store16_be(p, cnt);

    return p - dst.data();
}

size_t parse_read_bits_response(std::span<const uint8_t> src, function_code fc,
                                std::vector<bool>& coils, size_t cnt) {
    check_for_exception(src, fc);

    auto byte_cnt = bit_to_byte_count(cnt);
    validate_exact_rsp_length(src, read_bits_rsp_min_size + byte_cnt - 1);

    const uint8_t* p = src.data();
    function_code fc_rsp;
    size_t byte_cnt_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch8(byte_cnt_rsp, p);

    validate_field(fc_rsp == fc, "read_bits: function code invalid");
    validate_field(byte_cnt_rsp == byte_cnt, "read_bits: byte count invalid");

    p += parse_bits(src.subspan(p - src.data()), coils, cnt);

    return p - src.data();
}

size_t serialize_read_registers_request(std::span<uint8_t> dst,
                                        function_code fc,
                                        unsigned addr, size_t cnt) {
    validate_argument(addr, min_addr, max_addr, "read_registers: addr");
    validate_argument(
        cnt, min_read_registers, max_read_registers, "read_registers: cnt");

    expects(dst.size() >= read_registers_req_size, "buffer too small");

    uint8_t* p = dst.data();

    p += store8(p, fc);
    p += store16_be(p, addr);
    p += store16_be(p, cnt);

    return p - dst.data();
}


size_t parse_read_registers_response(std::span<const uint8_t> src,
                                     function_code fc,
                                     std::vector<uint16_t>& regs, size_t cnt) {
    check_for_exception(src, fc);

    validate_exact_rsp_length(src, read_registers_rsp_min_size +
                              sizeof(uint16_t) * (cnt - 1));

    const uint8_t* p = src.data();
    function_code fc_rsp;
    size_t byte_cnt_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch8(byte_cnt_rsp, p);

    validate_field(fc_rsp == fc, "read_registers: function code invalid");
    validate_field(byte_cnt_rsp == (cnt * sizeof(uint16_t)),
                   "read_registers: byte count invalid");

    p += parse_regs(src.subspan(p - src.data()), regs, cnt);

    return p - src.data();
}

} // namespace mboxid
