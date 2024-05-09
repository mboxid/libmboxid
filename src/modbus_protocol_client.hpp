// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP
#define LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP

#include <span>
#include <vector>
#include <mboxid/common.hpp>

namespace mboxid {

size_t serialize_read_coils_request(std::span<uint8_t> dst, unsigned addr,
                                    size_t cnt);


size_t parse_read_coils_response(std::span<const uint8_t> src,
                                 std::vector<bool>& coils, size_t cnt);

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_PROTOCOL_CLIENT_HPP
