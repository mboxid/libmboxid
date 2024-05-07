// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_PROTOCOL_SERVER_HPP
#define LIBMBOXID_MODBUS_PROTOCOL_SERVER_HPP

#include <span>
#include <mboxid/common.hpp>
#include <mboxid/backend_connector.hpp>

namespace mboxid {

std::size_t server_engine(backend_connector& backend,
                          std::span<const uint8_t> req, std::span<uint8_t> rsp);

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_PROTOCOL_SERVER_HPP