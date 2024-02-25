// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_TCP_SERVER_HPP
#define LIBMBOXID_MODBUS_TCP_SERVER_HPP

#include <memory>
#include <string_view>
#include <mboxid/error.hpp>
#include <mboxid/network.hpp>
#include <mboxid/backend_connector.hpp>

namespace mboxid {

class modbus_tcp_server {
public:
    using client_id = backend_connector::client_id;

    modbus_tcp_server();
    modbus_tcp_server(const modbus_tcp_server&) = delete;
    modbus_tcp_server& operator=(const modbus_tcp_server&) = delete;
    modbus_tcp_server(modbus_tcp_server&&) = default;
    modbus_tcp_server& operator=(modbus_tcp_server&&) = default;

    ~modbus_tcp_server();

    void set_server_addr(std::string_view host, std::string_view service = "",
                         net::ip_protocol_version ip_version =
                             net::ip_protocol_version::any);

    void set_backend(std::unique_ptr<backend_connector> backend);

    void run();
    void shutdown();
    void close_client_connection(client_id id);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace mbxodi

#endif // LIBMBOXID_MODBUS_TCP_SERVER_HPP