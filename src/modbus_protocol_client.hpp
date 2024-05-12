// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP
#define LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP

#include <span>
#include <vector>
#include <mboxid/common.hpp>

namespace mboxid {

size_t serialize_read_bits_request(
        std::span<uint8_t> dst, function_code fc, unsigned addr, size_t cnt);

size_t parse_read_bits_response(std::span<const uint8_t> src, function_code fc,
        std::vector<bool>& coils, size_t cnt);

size_t serialize_read_registers_request(
        std::span<uint8_t> dst, function_code fc, unsigned addr, size_t cnt);

size_t parse_read_registers_response(std::span<const uint8_t> src,
        function_code fc, std::vector<uint16_t>& coils, size_t cnt);

size_t serialize_write_single_coil_request(
        std::span<uint8_t> dst, unsigned addr, bool on);

size_t parse_write_single_coil_response(
        std::span<const uint8_t> src, unsigned addr, bool on);

size_t serialize_write_single_register_request(
        std::span<uint8_t> dst, unsigned addr, unsigned val);

size_t parse_write_single_register_response(
        std::span<const uint8_t> src, unsigned addr, unsigned val);

size_t serialize_write_multiple_coils_request(
        std::span<uint8_t> dst, unsigned addr, const std::vector<bool>& bits);

size_t parse_write_multiple_coils_response(
        std::span<const uint8_t> src, unsigned addr, size_t cnt);

size_t serialize_write_multiple_registers_request(std::span<uint8_t> dst,
        unsigned addr, const std::vector<uint16_t>& regs);

size_t parse_write_multiple_registers_response(
        std::span<const uint8_t> src, unsigned addr, size_t cnt);

size_t serialize_mask_write_register_request(std::span<uint8_t> dst,
        unsigned addr, unsigned and_msk, unsigned or_msk);

size_t parse_mask_write_register_response(std::span<const uint8_t> src,
        unsigned addr, unsigned and_msk, unsigned or_msk);

size_t serialize_read_write_multiple_registers_request(std::span<uint8_t> dst,
        unsigned addr_wr, const std::vector<uint16_t>& regs_wr,
        unsigned addr_rd, size_t cnt_rd);

size_t parse_read_write_multiple_registers_response(
        std::span<const uint8_t> src, std::vector<uint16_t>& regs, size_t cnt);

size_t serialize_read_device_identification_request(std::span<uint8_t> dst);

size_t parse_read_device_identification_response(std::span<const uint8_t> src,
        std::string& vendor, std::string& product, std::string& version);

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP
