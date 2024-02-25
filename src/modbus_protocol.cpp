// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <cstdint>
#include <map>
#include <type_traits>
#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol.hpp"

namespace mboxid {

using std::uint8_t;

enum class function_code : uint8_t {
    read_coils = 0x01,
    read_discrete_inputs = 0x02,
    read_holding_registers = 0x03,
    read_input_registers = 0x04,
    write_single_coil = 0x05,
    write_single_register = 0x06,
    write_multiple_coils = 0x0f,
    write_multiple_registers = 0x10,
    mask_write_register = 0x16,
    read_write_multiple_registers = 0x17,
    read_device_identification = 0x2b,
    exception = 0x80,
};

static const std::map<function_code, size_t> min_req_pdu_size_by_fc {
    {function_code::read_coils, 5},
    {function_code::read_discrete_inputs, 5},
    {function_code::read_holding_registers, 5},
    {function_code::read_input_registers, 5},
    {function_code::write_single_coil, 5},
    {function_code::write_single_register, 5},
    {function_code::write_multiple_coils, 7},
    {function_code::write_multiple_registers, 8},
    {function_code::mask_write_register, 7},
    {function_code::read_write_multiple_registers, 12},
    {function_code::read_device_identification, 4},
};

static const std::map<function_code, size_t> min_rsp_pdu_size_by_fc {
    {function_code::read_coils, 3},
    {function_code::read_discrete_inputs, 3},
    {function_code::read_holding_registers, 4},
    {function_code::read_input_registers, 4},
    {function_code::write_single_coil, 5},
    {function_code::write_single_register, 5},
    {function_code::write_multiple_coils, 5},
    {function_code::write_multiple_registers, 5},
    {function_code::mask_write_register, 7},
    {function_code::read_write_multiple_registers, 4},
    {function_code::read_device_identification, 16},
    {function_code::exception, 2},
};

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

static std::size_t serialize_exception_response(std::span<uint8_t> dst,
                                            function_code fc,  errc e) {
    expects(dst.size() >= 2, "buffer too small");

    uint8_t* p = dst.data();
    p += store8(p, static_cast<uint8_t>(function_code::exception) |
                       static_cast<uint8_t>(fc));
    p += store8(p, e);
    return p - dst.data();
}

std::size_t server_engine(backend_connector& backend,
                          std::span<const uint8_t> req,
                          std::span<uint8_t> rsp)
{
    auto fc = static_cast<function_code>(req[0]);
    return serialize_exception_response(
                rsp, fc, errc::modbus_exception_illegal_function);

}

} // namespace mboxid
