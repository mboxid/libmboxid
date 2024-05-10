// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP
#define LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP

#include <span>
#include <vector>
#include <mboxid/common.hpp>

namespace mboxid {

size_t serialize_read_bits_request(std::span<uint8_t> dst, function_code fc,
                                   unsigned addr, size_t cnt);


size_t parse_read_bits_response(std::span<const uint8_t> src, function_code fc,
                                 std::vector<bool>& coils, size_t cnt);

size_t serialize_read_registers_request(std::span<uint8_t> dst,
                                        function_code fc,
                                        unsigned addr, size_t cnt);


size_t parse_read_registers_response(std::span<const uint8_t> src,
                                     function_code fc,
                                     std::vector<uint16_t>& coils, size_t cnt);

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP
