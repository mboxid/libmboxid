// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
#include <cstring>
#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_server.hpp"

namespace mboxid {

static void validate_min_req_length(std::span<const uint8_t> req, size_t len)
{
    if (req.size() < len)
        throw mboxid_error(errc::parse_error, "request length too small");
}

static void validate_exact_req_length(std::span<const uint8_t> req, size_t len)
{
    if (req.size() != len)
        throw mboxid_error(errc::parse_error, "request length invalid");
}

template <typename T>
bool is_in_range(T val, T min, T max) {
    return (val >= min) && (val <= max);
}

static size_t serialize_exception_response(std::span<uint8_t> dst,
                                           function_code fc,  errc e) {
    expects(dst.size() >= 2, "buffer too small");

    uint8_t* p = dst.data();
    p += store8(p, static_cast<uint8_t>(function_code::exception) |
                   static_cast<uint8_t>(fc));
    p += store8(p, e);
    return p - dst.data();
}

static size_t process_read_bits(backend_connector& backend,
                     std::span<const uint8_t> req, std::span<uint8_t> rsp) {
    // parse request
    validate_exact_req_length(req, read_bits_req_size);

    function_code fc;
    unsigned addr;
    size_t cnt;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(cnt, p_req);

    if (!is_in_range(cnt, min_read_bits, max_read_bits))
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_data_value);

    // invoke backend
    std::vector<bool> bits;
    bits.reserve(cnt);

    mboxid::errc res;

    if (fc == function_code::read_coils)
        res = backend.read_coils(addr, cnt, bits);
    else
        res = backend.read_discrete_inputs(addr, cnt, bits);

    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend read coils _or_ discrete inputs");

    expects(bits.size() == cnt, "backend returned wrong number of bits");

    // serialize response
    auto byte_cnt = bit_to_byte_count(cnt);
    expects(rsp.size() >= (read_bits_rsp_min_size + byte_cnt - 1),
            "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store8(p_rsp, byte_cnt);
    p_rsp += serialize_bits(rsp.subspan(p_rsp - rsp.data()), bits);

    return p_rsp - rsp.data();
}

static size_t process_read_registers(backend_connector& backend,
                                std::span<const uint8_t> req, std::span<uint8_t> rsp) {
    // parse request
    validate_exact_req_length(req, read_registers_req_size);

    function_code fc;
    unsigned addr;
    size_t cnt;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(cnt, p_req);

    if (!is_in_range(cnt, min_read_registers, max_read_registers))
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_data_value);

    // invoke backend
    std::vector<uint16_t> regs;
    regs.reserve(cnt);

    mboxid::errc res;

    if (fc == function_code::read_holding_registers)
        res = backend.read_holding_registers(addr, cnt, regs);
    else
        res = backend.read_input_registers(addr, cnt, regs);

    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend read holding _or_ input registers");

    expects(regs.size() == cnt, "backend returned wrong number of registers");

    // serialize response
    auto byte_cnt = cnt * sizeof(uint16_t);
    expects(rsp.size() >= (read_registers_rsp_min_size + byte_cnt -
                           sizeof(uint16_t)), "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store8(p_rsp, byte_cnt);
    p_rsp += serialize_regs(rsp.subspan(p_rsp - rsp.data()), regs);

    return p_rsp - rsp.data();
}

static size_t process_write_single_coil(backend_connector& backend,
                                        std::span<const uint8_t> req,
                                        std::span<uint8_t> rsp) {
    // parse request
    validate_exact_req_length(req, write_coil_req_size);

    function_code fc;
    unsigned addr;
    unsigned val;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(val, p_req);

    if ((val != single_coil_off) && (val != single_coil_on))
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_data_value);

    // invoke backend connector
    std::vector<bool> bits {val == single_coil_on};

    auto res = backend.write_coils(addr, bits);
    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend write coils");

    // serialize response
    expects(rsp.size() >= write_coil_rsp_size, "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store16_be(p_rsp, addr);
    p_rsp += store16_be(p_rsp, val);

    return p_rsp - rsp.data();
}

static size_t process_write_single_register(backend_connector& backend,
                                        std::span<const uint8_t> req,
                                        std::span<uint8_t> rsp) {
    // parse request
    validate_exact_req_length(req, write_register_req_size);

    function_code fc;
    unsigned addr;
    uint16_t val;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(val, p_req);

    // invoke backend connector
    std::vector<uint16_t> regs{val};

    auto res = backend.write_holding_registers(addr, regs);
    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend write holding registers");

    // serialize response
    expects(rsp.size() >= write_register_rsp_size, "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store16_be(p_rsp, addr);
    p_rsp += store16_be(p_rsp, val);

    return p_rsp - rsp.data();
}

static size_t process_write_multiple_coils(backend_connector& backend,
                                            std::span<const uint8_t> req,
                                            std::span<uint8_t> rsp) {
    // parse request
    validate_min_req_length(req, write_multiple_coils_req_min_size);

    function_code fc;
    unsigned addr;
    size_t cnt;
    size_t byte_cnt;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(cnt, p_req);
    p_req += fetch8(byte_cnt, p_req);

    if (!is_in_range(cnt, min_write_coils, max_write_coils) ||
        (byte_cnt != bit_to_byte_count(cnt)))
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_data_value);

    std::vector<bool> bits;
    p_req += parse_bits(req.subspan(p_req - req.data()), bits, cnt);

    // invoke backend connector
    auto res = backend.write_coils(addr, bits);
    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend write coils");

    // serialize response
    expects(rsp.size() >= write_multiple_coils_rsp_size, "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store16_be(p_rsp, addr);
    p_rsp += store16_be(p_rsp, cnt);

    return p_rsp - rsp.data();
}

static size_t process_write_multiple_registers(backend_connector& backend,
                                           std::span<const uint8_t> req,
                                           std::span<uint8_t> rsp) {
    // parse request
    validate_min_req_length(req, write_multiple_registers_req_min_size);

    function_code fc;
    unsigned addr;
    size_t cnt;
    size_t byte_cnt;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(cnt, p_req);
    p_req += fetch8(byte_cnt, p_req);

    if (!is_in_range(cnt, min_write_registers, max_write_registers) ||
        (byte_cnt != (cnt * sizeof(uint16_t))))
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_data_value);

    std::vector<uint16_t> regs;
    p_req += parse_regs(req.subspan(p_req - req.data()), regs, cnt);

    // invoke backend connector
    auto res = backend.write_holding_registers(addr, regs);
    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend write coils");

    // serialize response
    expects(rsp.size() >= write_multiple_registers_rsp_size, "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store16_be(p_rsp, addr);
    p_rsp += store16_be(p_rsp, cnt);

    return p_rsp - rsp.data();
}

static size_t process_mask_write_registers(backend_connector& backend,
                                               std::span<const uint8_t> req,
                                               std::span<uint8_t> rsp) {
    // parse request
    validate_exact_req_length(req, mask_write_register_req_size);

    function_code fc;
    unsigned addr;
    unsigned and_mask;
    unsigned or_mask;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr, p_req);
    p_req += fetch16_be(and_mask, p_req);
    p_req += fetch16_be(or_mask, p_req);

    // invoke backend
    std::vector<uint16_t> regs;
    auto res = backend.read_holding_registers(addr, 1, regs);

    if (res == errc::none) {
        expects(regs.size() == 1, "backend returned wrong number of registers");
        regs[0] = (regs[0] & and_mask) | (or_mask & ~and_mask);
        res = backend.write_holding_registers(addr, regs);
    }

    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend read _or_ write holding registers");

    // serialize response
    expects(rsp.size() >= mask_write_register_rsp_size, "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store16_be(p_rsp, addr);
    p_rsp += store16_be(p_rsp, and_mask);
    p_rsp += store16_be(p_rsp, or_mask);

    return p_rsp - rsp.data();
}

static size_t
process_read_write_multiple_registers(backend_connector& backend,
                                               std::span<const uint8_t> req,
                                               std::span<uint8_t> rsp) {
    // parse request
    validate_min_req_length(req, read_write_multiple_registers_req_min_size);

    function_code fc;
    unsigned addr_rd;
    size_t cnt_rd;
    unsigned addr_wr;
    size_t cnt_wr;
    size_t byte_cnt_wr;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch16_be(addr_rd, p_req);
    p_req += fetch16_be(cnt_rd, p_req);
    p_req += fetch16_be(addr_wr, p_req);
    p_req += fetch16_be(cnt_wr, p_req);
    p_req += fetch8(byte_cnt_wr, p_req);

    if (!is_in_range(
            cnt_rd, min_rdwr_read_registers, max_rdwr_read_registers) ||
        !is_in_range(
            cnt_wr, min_rdwr_write_registers, max_rdwr_write_registers) ||
        (byte_cnt_wr != (cnt_wr * sizeof(uint16_t))))
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_data_value);

    std::vector<uint16_t> regs_wr;
    p_req += parse_regs(req.subspan(p_req - req.data()), regs_wr, cnt_wr);

    // invoke backend connector
    std::vector<uint16_t> regs_rd;
    regs_rd.reserve(cnt_rd);
    auto res = backend.write_read_holding_registers(
                    addr_wr, regs_wr, addr_rd, cnt_rd, regs_rd);
    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend write/read coils");

    expects(regs_rd.size() == cnt_rd,
            "backend returned wrong number of registers");

    // serialize response
    auto byte_cnt_rd = cnt_rd * sizeof(uint16_t);
    expects(rsp.size() >= (read_write_multiple_registers_rsp_min_size +
                           byte_cnt_rd - sizeof(uint16_t)), "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store8(p_rsp, byte_cnt_rd);
    p_rsp += serialize_regs(rsp.subspan(p_rsp - rsp.data()), regs_rd);

    return p_rsp - rsp.data();
}

static size_t process_read_device_information(backend_connector& backend,
                                                    std::span<const uint8_t> req,
                                                    std::span<uint8_t> rsp) {
    // parse request
    validate_exact_req_length(req, read_device_identification_req_size);

    function_code fc;
    mei_type mei;
    read_device_id_code code;
    object_id id;

    auto* p_req = req.data();
    p_req += fetch8(fc, p_req);
    p_req += fetch8(mei, p_req);
    p_req += fetch8(code, p_req);
    p_req += fetch8(id, p_req);

    if ((mei != mei_type::modbus) || (code != read_device_id_code::basic))
        return serialize_exception_response(rsp, fc,
                                errc::modbus_exception_illegal_data_value);
    if (id != object_id::vendor_name)
        return serialize_exception_response(rsp, fc,
                                errc::modbus_exception_illegal_data_address);

    // invoke backend
    std::string vendor, product, version;
    auto res = backend.get_basic_device_identification(vendor, product,
                                                       version);

    if (is_modbus_exception(res))
        return serialize_exception_response(rsp, fc, res);
    else if (res != errc::none)
        throw mboxid_error(res, "backend device identification");

    // serialize response
    expects(rsp.size() >=
                (read_device_identification_rsp_min_size
                    + (vendor.length() - 1)
                    + 2 + product.length()
                    + 2 + version.length()), "buffer too small");

    auto p_rsp = rsp.data();
    p_rsp += store8(p_rsp, fc);
    p_rsp += store8(p_rsp, mei_type::modbus);
    p_rsp += store8(p_rsp, code);
    p_rsp += store8(p_rsp, read_device_id_code::basic); // conformity level
    p_rsp += store8(p_rsp, 0x00);   // more follows: no
    p_rsp += store8(p_rsp, 0x00);   // next object id
    p_rsp += store8(p_rsp, 0x03);   // number of objects
    p_rsp += store8(p_rsp, object_id::vendor_name);
    p_rsp += store8(p_rsp, vendor.length());
    std::memcpy(p_rsp, vendor.c_str(), vendor.length());
    p_rsp += vendor.length();
    p_rsp += store8(p_rsp, object_id::product_code);
    p_rsp += store8(p_rsp, product.length());
    std::memcpy(p_rsp, product.c_str(), product.length());
    p_rsp += product.length();
    p_rsp += store8(p_rsp, object_id::major_minor_revision);
    p_rsp += store8(p_rsp, version.length());
    std::memcpy(p_rsp, version.c_str(), version.length());
    p_rsp += version.length();

    return p_rsp - rsp.data();
}

size_t server_engine(backend_connector& backend,
                     std::span<const uint8_t> req, std::span<uint8_t> rsp) {
    validate_min_req_length(req, min_pdu_size);

    auto fc = static_cast<function_code>(req[0]);
    switch (fc) {
    case function_code::read_coils: [[fallthrough]];
    case function_code::read_discrete_inputs:
        return process_read_bits(backend, req, rsp);
    case function_code::read_holding_registers: [[fallthrough]];
    case function_code::read_input_registers:
        return process_read_registers(backend, req, rsp);
    case function_code::write_single_coil:
        return process_write_single_coil(backend, req, rsp);
    case function_code::write_single_register:
        return process_write_single_register(backend, req, rsp);
    case function_code::write_multiple_coils:
        return process_write_multiple_coils(backend, req, rsp);
    case function_code::write_multiple_registers:
        return process_write_multiple_registers(backend, req, rsp);
    case function_code::mask_write_register:
        return process_mask_write_registers(backend, req, rsp);
    case function_code::read_write_multiple_registers:
        return process_read_write_multiple_registers(backend, req, rsp);
    case function_code::read_device_identification:
        return process_read_device_information(backend, req, rsp);
    default:
        return serialize_exception_response(
            rsp, fc, errc::modbus_exception_illegal_function);
    }
}

} // namespace mboxid
