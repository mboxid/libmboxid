// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_PROTOCOL_HPP
#define LIBMBOXID_MODBUS_PROTOCOL_HPP

#include <cstdint>
#include <span>
#include <mboxid/backend_connector.hpp>

namespace mboxid {

constexpr std::size_t min_pdu_size{1};
constexpr std::size_t max_pdu_size{253};
constexpr std::size_t mbap_header_size{7};
constexpr std::size_t max_adu_size{mbap_header_size + max_pdu_size};

struct mbap_header {
    std::uint16_t transaction_id;
    std::uint16_t protocol_id;
    std::uint16_t length;
    std::uint8_t unit_id;
};

static inline std::size_t get_pdu_size(const mbap_header& header) {
    return header.length - sizeof(header.unit_id);
}

static inline std::size_t get_adu_size(const mbap_header& header) {
    return mbap_header_size + get_pdu_size(header);
}

void parse_mbap_header(std::span<const std::uint8_t> src, mbap_header& header);

size_t serialize_mbap_header(std::span<std::uint8_t> dst,
                             const mbap_header& header);

std::size_t server_engine(backend_connector& backend,
                          std::span<const std::uint8_t> req,
                          std::span<uint8_t> rsp);


} // namespace mboxid

#endif // LIBMBOXID_MODBUS_PROTOCOL_HPP
