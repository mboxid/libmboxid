// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_PROTOCOL_COMMON_HPP
#define LIBMBOXID_MODBUS_PROTOCOL_COMMON_HPP

#include <span>
#include <vector>
#include <limits>
#include <mboxid/common.hpp>

namespace mboxid {

/**
 * Minimum PDU size.
 * The smallest PDU is that of an exception response.
 */
constexpr size_t min_pdu_size{2};

constexpr size_t max_pdu_size{253};
constexpr size_t mbap_header_size{7};
constexpr size_t max_adu_size{mbap_header_size + max_pdu_size};

constexpr size_t read_bits_req_size{5};
constexpr size_t read_bits_rsp_min_size{3};
constexpr size_t read_registers_req_size{5};
constexpr size_t read_registers_rsp_min_size{4};
constexpr size_t write_coil_req_size{5};
constexpr size_t write_coil_rsp_size{5};
constexpr size_t write_register_req_size{5};
constexpr size_t write_register_rsp_size{5};
constexpr size_t write_multiple_coils_req_min_size{7};
constexpr size_t write_multiple_coils_rsp_size{5};
constexpr size_t write_multiple_registers_req_min_size{8};
constexpr size_t write_multiple_registers_rsp_size{5};
constexpr size_t mask_write_register_req_size{7};
constexpr size_t mask_write_register_rsp_size{7};
constexpr size_t read_write_multiple_registers_req_min_size{12};
constexpr size_t read_write_multiple_registers_rsp_min_size{4};
constexpr size_t read_device_identification_req_size{4};
constexpr size_t read_device_identification_rsp_min_size{10};
constexpr size_t exception_rsp_size{2};

constexpr unsigned single_coil_off{0x0000};
constexpr unsigned single_coil_on{0xff00};

// Modbus_Application_Protocol_V1_1b3.pdf
// Section 6.1: Quantity of coils to read: 1 to 2000 (0x7d0).
// Section 6.2: Quantity of discrete inputs to read: 1 to 2000 (0x7d0).
constexpr size_t min_read_bits{1};
constexpr size_t max_read_bits{2000};

// Modbus_Application_Protocol_V1_1b3.pdf
// Section 6.3: Quantity of holding registers to read: 1 to 125 (0x7d).
// Section 6.4: Quantity of inputs registers to read: 1 to 125 (0x7d).
constexpr size_t min_read_registers{1};
constexpr size_t max_read_registers{125};

// Modbus_Application_Protocol_V1_1b3.pdf
// Section 6.11: Quantity of coils to write: 1 to 1968 (0x7b0).
constexpr size_t min_write_coils{1};
constexpr size_t max_write_coils{1968};

// Modbus_Application_Protocol_V1_1b3.pdf
// Section 6.12: Quantity of holding registers to write: 1 to 123 (0x7b)
constexpr size_t min_write_registers{1};
constexpr size_t max_write_registers{123};

// Modbus_Application_Protocol_V1_1b3.pdf
// Section 6.17, read/write multiple registers:
//  - Quantity of holding registers to read : 1 to 125 (0x7d).
//  - Quantity of holding registers to write : 1 to 121 (0x79).
constexpr size_t min_rdwr_read_registers{1};
constexpr size_t max_rdwr_read_registers{125};
constexpr size_t min_rdwr_write_registers{1};
constexpr size_t max_rdwr_write_registers{121};

constexpr auto bits_per_byte = std::numeric_limits<uint8_t>::digits;

struct mbap_header {
    uint16_t transaction_id;
    uint16_t protocol_id;
    uint16_t length;
    uint8_t unit_id;
};

enum class function_code {
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

enum class object_id {
    vendor_name = 0x00,
    product_code = 0x01,
    major_minor_revision = 0x02,
};

enum class read_device_id_code {
    basic = 0x01,
};

enum class mei_type {
    modbus = 0x0e,
};

static inline size_t get_pdu_size(const mbap_header& header) {
    return header.length - sizeof(header.unit_id);
}

/*
 * CLion complains:
 *      All calls of function 'get_pdu_size' are unreachable
 * Actually, this is a false positive.
 */
static inline size_t get_adu_size(const mbap_header& header) {
    return mbap_header_size + get_pdu_size(header);
}

static inline void set_pdu_size(mbap_header& header, size_t pdu_size) {
    header.length = sizeof(header.unit_id) + pdu_size;
}

static inline size_t bit_to_byte_count(size_t n_bits) {
    return (n_bits + bits_per_byte - 1) / bits_per_byte;
}

void parse_mbap_header(std::span<const uint8_t> src, mbap_header& header);

size_t serialize_mbap_header(std::span<uint8_t> dst, const mbap_header& header);

size_t parse_bits(
        std::span<const uint8_t> src, std::vector<bool>& bits, size_t cnt);

size_t serialize_bits(std::span<uint8_t> dst, const std::vector<bool>& bits);

size_t parse_regs(
        std::span<const uint8_t> src, std::vector<uint16_t>& regs, size_t cnt);

size_t serialize_regs(
        std::span<uint8_t> dst, const std::vector<uint16_t>& regs);

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_PROTOCOL_COMMON_HPP
