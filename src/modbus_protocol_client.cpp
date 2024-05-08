// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_client.hpp"

namespace mboxid {

constexpr unsigned min_addr = 0U;
constexpr unsigned max_addr = 0xffffU;

size_t serialize_read_coils_request(std::span<uint8_t> dst, unsigned addr,
                                    size_t cnt)
{
    validate_argument(addr, min_addr, max_addr, "read_coils: addr");
    validate_argument(cnt, min_read_bits, max_read_bits, "read_coils: cnt");

    expects(dst.size() >= read_bits_req_size, "buffer too small");
}

} // namespace mboxid
