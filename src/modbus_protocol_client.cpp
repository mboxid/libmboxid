// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_client.hpp"

namespace mboxid {

static_assert(sizeof(unsigned) >= sizeof(uint16_t), "unsigned type too small");

constexpr unsigned u16_min = 0U;
constexpr unsigned u16_max = 0xffffU;

static void validate_exact_rsp_length(
        std::span<const uint8_t> rsp, size_t len) {
    if (rsp.size() != len)
        throw mboxid_error(errc::parse_error, "response wrong length");
}

void validate_field(bool cond, const char* msg) {
    if (!cond)
        throw mboxid_error(errc::parse_error, msg);
}

static void check_for_exception(
        std::span<const uint8_t> rsp, function_code fc) {
    if (rsp.size() != exception_rsp_size)
        return;

    auto msk = static_cast<unsigned>(function_code::exception);
    unsigned fc_rsp = rsp[0];
    unsigned exception_code = rsp[1];

    if (!(fc_rsp & msk))
        return;

    validate_field((fc_rsp & ~msk) == static_cast<unsigned>(fc),
            "modbus exception: function code");

    auto err = static_cast<errc>(exception_code);
    validate_field(is_modbus_exception(err), "modbus exception: code");

    // throw valid modbus exception as mboxid_error
    throw mboxid_error(err);
}

size_t serialize_read_bits_request(
        std::span<uint8_t> dst, function_code fc, unsigned addr, size_t cnt) {
    validate_argument(addr, u16_min, u16_max, "addr");
    validate_argument(cnt, min_read_bits, max_read_bits, "cnt");
    expects(dst.size() >= read_bits_req_size, "buffer too small");

    auto p = dst.data();

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

    auto p = src.data();
    function_code fc_rsp;
    size_t byte_cnt_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch8(byte_cnt_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(byte_cnt_rsp == byte_cnt, "byte count");

    p += parse_bits(src.subspan(p - src.data()), coils, cnt);

    return p - src.data();
}

size_t serialize_read_registers_request(
        std::span<uint8_t> dst, function_code fc, unsigned addr, size_t cnt) {
    validate_argument(addr, u16_min, u16_max, "addr");
    validate_argument(cnt, min_read_registers, max_read_registers, "cnt");
    expects(dst.size() >= read_registers_req_size, "buffer too small");

    auto p = dst.data();

    p += store8(p, fc);
    p += store16_be(p, addr);
    p += store16_be(p, cnt);

    return p - dst.data();
}

size_t parse_read_registers_response(std::span<const uint8_t> src,
        function_code fc, std::vector<uint16_t>& regs, size_t cnt) {
    check_for_exception(src, fc);

    validate_exact_rsp_length(
            src, read_registers_rsp_min_size + sizeof(uint16_t) * (cnt - 1));

    auto p = src.data();
    function_code fc_rsp;
    size_t byte_cnt_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch8(byte_cnt_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(byte_cnt_rsp == (cnt * sizeof(uint16_t)), "byte count");

    p += parse_regs(src.subspan(p - src.data()), regs, cnt);

    return p - src.data();
}

size_t serialize_write_single_coil_request(
        std::span<uint8_t> dst, unsigned addr, bool on) {
    validate_argument(addr, u16_min, u16_max, "addr");
    expects(dst.size() >= write_coil_req_size, "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::write_single_coil);
    p += store16_be(p, addr);
    p += store16_be(p, on ? single_coil_on : single_coil_off);

    return p - dst.data();
}

size_t parse_write_single_coil_response(
        std::span<const uint8_t> src, unsigned addr, bool on) {
    constexpr auto fc = function_code::write_single_coil;
    check_for_exception(src, fc);

    validate_exact_rsp_length(src, write_coil_rsp_size);

    auto p = src.data();
    function_code fc_rsp;
    unsigned addr_rsp;
    unsigned val_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch16_be(addr_rsp, p);
    p += fetch16_be(val_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(addr_rsp == addr, "addr");
    validate_field(val_rsp == (on ? single_coil_on : single_coil_off), "value");

    return p - src.data();
}

size_t serialize_write_single_register_request(
        std::span<uint8_t> dst, unsigned addr, unsigned val) {
    validate_argument(addr, u16_min, u16_max, "addr");
    validate_argument(val, u16_min, u16_max, "val");
    expects(dst.size() >= write_register_req_size, "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::write_single_register);
    p += store16_be(p, addr);
    p += store16_be(p, val);

    return p - dst.data();
}

size_t parse_write_single_register_response(
        std::span<const uint8_t> src, unsigned addr, unsigned val) {
    constexpr auto fc = function_code::write_single_register;
    check_for_exception(src, fc);

    validate_exact_rsp_length(src, write_register_rsp_size);

    auto p = src.data();
    function_code fc_rsp;
    unsigned addr_rsp;
    unsigned val_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch16_be(addr_rsp, p);
    p += fetch16_be(val_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(addr_rsp == addr, "addr");
    validate_field(val_rsp == val, "value");

    return p - src.data();
}

size_t serialize_write_multiple_coils_request(
        std::span<uint8_t> dst, unsigned addr, const std::vector<bool>& bits) {
    validate_argument(addr, u16_min, u16_max, "addr");
    auto cnt = bits.size();
    validate_argument(cnt, min_write_coils, max_write_coils, "cnt");
    auto byte_cnt = bit_to_byte_count(cnt);
    expects(dst.size() >= write_multiple_coils_req_min_size + byte_cnt - 1,
            "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::write_multiple_coils);
    p += store16_be(p, addr);
    p += store16_be(p, cnt);
    p += store8(p, byte_cnt);
    p += serialize_bits(dst.subspan(p - dst.data()), bits);

    return p - dst.data();
}

size_t parse_write_multiple_coils_response(
        std::span<const uint8_t> src, unsigned addr, size_t cnt) {
    constexpr auto fc = function_code::write_multiple_coils;
    check_for_exception(src, fc);

    validate_exact_rsp_length(src, write_multiple_coils_rsp_size);

    auto p = src.data();
    function_code fc_rsp;
    unsigned addr_rsp;
    size_t cnt_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch16_be(addr_rsp, p);
    p += fetch16_be(cnt_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(addr_rsp == addr, "addr");
    validate_field(cnt_rsp == cnt, "count");

    return p - src.data();
}

size_t serialize_write_multiple_registers_request(std::span<uint8_t> dst,
        unsigned addr, const std::vector<uint16_t>& regs) {
    validate_argument(addr, u16_min, u16_max, "addr");
    auto cnt = regs.size();
    validate_argument(cnt, min_write_registers, max_write_registers, "cnt");
    expects(dst.size() >= (write_multiple_registers_req_min_size +
                                  sizeof(uint16_t) * (cnt - 1)),
            "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::write_multiple_registers);
    p += store16_be(p, addr);
    p += store16_be(p, cnt);
    p += store8(p, cnt * sizeof(uint16_t));
    p += serialize_regs(dst.subspan(p - dst.data()), regs);

    return p - dst.data();
}

size_t parse_write_multiple_registers_response(
        std::span<const uint8_t> src, unsigned addr, size_t cnt) {
    constexpr auto fc = function_code::write_multiple_registers;
    check_for_exception(src, fc);

    validate_exact_rsp_length(src, write_multiple_registers_rsp_size);

    auto p = src.data();
    function_code fc_rsp;
    unsigned addr_rsp;
    size_t cnt_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch16_be(addr_rsp, p);
    p += fetch16_be(cnt_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(addr_rsp == addr, "addr");
    validate_field(cnt_rsp == cnt, "count");

    return p - src.data();
}

size_t serialize_mask_write_register_request(std::span<uint8_t> dst,
        unsigned addr, unsigned and_msk, unsigned or_msk) {
    validate_argument(addr, u16_min, u16_max, "addr");
    validate_argument(and_msk, u16_min, u16_max, "and_msk");
    validate_argument(or_msk, u16_min, u16_max, "or_msk");
    expects(dst.size() >= mask_write_register_req_size, "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::mask_write_register);
    p += store16_be(p, addr);
    p += store16_be(p, and_msk);
    p += store16_be(p, or_msk);

    return p - dst.data();
}

size_t parse_mask_write_register_response(std::span<const uint8_t> src,
        unsigned addr, unsigned and_msk, unsigned or_msk) {
    constexpr auto fc = function_code::mask_write_register;
    check_for_exception(src, fc);

    validate_exact_rsp_length(src, mask_write_register_rsp_size);

    auto p = src.data();
    function_code fc_rsp;
    unsigned addr_rsp;
    unsigned and_msk_rsp;
    unsigned or_msk_rsp;

    p += fetch8(fc_rsp, p);
    p += fetch16_be(addr_rsp, p);
    p += fetch16_be(and_msk_rsp, p);
    p += fetch16_be(or_msk_rsp, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(addr_rsp == addr, "addr");
    validate_field(and_msk_rsp == and_msk, "and mask");
    validate_field(or_msk_rsp == or_msk, "or mask");

    return p - src.data();
}

size_t serialize_read_write_multiple_registers_request(std::span<uint8_t> dst,
        unsigned addr_wr, const std::vector<uint16_t>& regs_wr,
        unsigned addr_rd, size_t cnt_rd) {
    validate_argument(addr_wr, u16_min, u16_max, "addr_wr");
    validate_argument(addr_rd, u16_min, u16_max, "addr_rd");
    auto cnt_wr = regs_wr.size();
    validate_argument(cnt_wr, min_rdwr_write_registers,
            max_rdwr_write_registers, "cnt_wr");
    validate_argument(
            cnt_rd, min_rdwr_read_registers, max_rdwr_read_registers, "cnt_rd");
    expects(dst.size() >= (read_write_multiple_registers_req_min_size +
                                  sizeof(uint16_t) * (cnt_wr - 1)),
            "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::read_write_multiple_registers);
    p += store16_be(p, addr_rd);
    p += store16_be(p, cnt_rd);
    p += store16_be(p, addr_wr);
    p += store16_be(p, cnt_wr);
    p += store8(p, cnt_wr * sizeof(uint16_t));
    p += serialize_regs(dst.subspan(p - dst.data()), regs_wr);

    return p - dst.data();
}

size_t parse_read_write_multiple_registers_response(
        std::span<const uint8_t> src, std::vector<uint16_t>& regs, size_t cnt) {
    static_assert(read_write_multiple_registers_rsp_min_size ==
                          read_registers_rsp_min_size,
            "incompatible response type");
    return parse_read_registers_response(
            src, function_code::read_write_multiple_registers, regs, cnt);
}

size_t serialize_read_device_identification_request(std::span<uint8_t> dst) {
    expects(dst.size() >= read_device_identification_req_size,
            "buffer too small");

    auto p = dst.data();

    p += store8(p, function_code::read_device_identification);
    p += store8(p, mei_type::modbus);
    p += store8(p, read_device_id_code::basic);
    p += store8(p, object_id::vendor_name);

    return p - dst.data();
}

size_t parse_read_device_identification_response(std::span<const uint8_t> src,
        std::string& vendor, std::string& product, std::string& version) {
    constexpr auto fc = function_code::read_device_identification;
    check_for_exception(src, fc);

    if (src.size() < read_device_identification_rsp_min_size)
        throw mboxid_error(errc::parse_error, "response too short");

    auto p = src.data();
    function_code fc_rsp;
    mei_type mei_type_rsp;
    read_device_id_code id_code_rsp;
    unsigned more;
    size_t number_of_objects;

    p += fetch8(fc_rsp, p);
    p += fetch8(mei_type_rsp, p);
    p += fetch8(id_code_rsp, p);
    p++; // ignore conformity level
    p += fetch8(more, p);
    p++; // ignore next object id
    p += fetch8(number_of_objects, p);

    validate_field(fc_rsp == fc, "function code");
    validate_field(mei_type_rsp == mei_type::modbus, "mei type");
    validate_field(id_code_rsp == read_device_id_code::basic, "id code");
    validate_field(more == 0, "more");
    validate_field(number_of_objects == 3, "number of objects");

    auto left = [&](auto p) { return src.size() - (p - src.data()); };
    object_id oid;
    size_t olen;
    for (size_t i = 0; i < number_of_objects; ++i) {
        if (left(p) < 2)
            throw mboxid_error(errc::parse_error, "object id/len incomplete");
        p += fetch8(oid, p);
        p += fetch8(olen, p);
        if (left(p) < olen)
            throw mboxid_error(errc::parse_error, "object value incomplete");
        switch (oid) {
        case object_id::vendor_name:
            vendor.assign(p, p + olen);
            break;
        case object_id::product_code:
            product.assign(p, p + olen);
            break;
        case object_id::major_minor_revision:
            version.assign(p, p + olen);
            break;
        default:
            throw mboxid_error(errc::parse_error, "object id");
        }
        p += olen;
    }

    return p - src.data();
}

} // namespace mboxid
