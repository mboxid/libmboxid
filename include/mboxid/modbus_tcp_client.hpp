// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_TCP_CLIENT_HPP
#define LIBMBOXID_MODBUS_TCP_CLIENT_HPP

#include <memory>
#include <string>
#include <vector>
#include <mboxid/common.hpp>
#include <mboxid/error.hpp>
#include <mboxid/network.hpp>

namespace mboxid {

class modbus_tcp_client {
public:
    struct context;

    modbus_tcp_client();
    modbus_tcp_client(const modbus_tcp_client&) = delete;
    modbus_tcp_client& operator=(const modbus_tcp_client&) = delete;
    modbus_tcp_client(modbus_tcp_client&&) = default;
    modbus_tcp_client& operator=(modbus_tcp_client&&) = default;

    ~modbus_tcp_client();

    void connect_to_server(const std::string& host,
                           const std::string& service = "",
                           net::ip_protocol_version ip_version =
                                net::ip_protocol_version::any,
                           milliseconds timeout = no_timeout);

    void set_response_timeout(milliseconds timeout);
    void set_unit_id(unsigned id);

    std::vector<bool> read_coils(unsigned address, size_t cnt);

private:
    std::unique_ptr<context> ctx;
};

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_TCP_CLIENT_HPP
