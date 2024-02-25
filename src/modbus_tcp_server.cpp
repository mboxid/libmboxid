// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <mboxid/modbus_tcp_server.hpp>
#include "modbus_tcp_server_impl.hpp"

namespace mboxid {

modbus_tcp_server::modbus_tcp_server() : pimpl(std::make_unique<impl>()) {}

void modbus_tcp_server::set_server_addr(std::string_view host,
                                        std::string_view service,
                                        net::ip_protocol_version ip_version) {
    pimpl->set_server_addr(host, service, ip_version);
}

void modbus_tcp_server::set_backend(
    std::unique_ptr<backend_connector> backend) {
    pimpl->set_backend(std::move(backend));
}

void modbus_tcp_server::run() {
    pimpl->run();
}

void modbus_tcp_server::shutdown() {
    pimpl->shutdown();
}

void modbus_tcp_server::close_client_connection(client_id id) {
    pimpl->close_client_connection(id);
}

modbus_tcp_server::~modbus_tcp_server() = default;

} // namespace mboxid
