// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <mboxid/modbus_tcp_server.hpp>
#include "modbus_tcp_server_impl.hpp"

namespace mboxid {

modbus_tcp_server::modbus_tcp_server() : pimpl(std::make_unique<impl>()) {}

modbus_tcp_server::~modbus_tcp_server() = default;

void modbus_tcp_server::set_server_addr(const std::string& host,
                                        const std::string& service,
                                        net::ip_protocol_version ip_version) {
    pimpl->set_server_addr(host, service, ip_version);
}

void modbus_tcp_server::set_backend(
    std::unique_ptr<backend_connector> backend) {
    pimpl->set_backend(std::move(backend));
}

backend_connector* modbus_tcp_server::borrow_backend() {
    return pimpl->borrow_backend();
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

void modbus_tcp_server::set_idle_timeout(milliseconds to) {
    pimpl->set_idle_timeout(to);
}

void modbus_tcp_server::set_request_complete_timeout(milliseconds to) {
    pimpl->set_request_complete_timeout(to);
}

} // namespace mboxid
