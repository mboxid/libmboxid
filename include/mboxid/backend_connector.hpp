// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_BACKEND_CONNECTOR_HPP
#define LIBMBOXID_BACKEND_CONNECTOR_HPP

#include <iostream>
#include <cstdint>
#include <chrono>
#include <vector>
#include <mboxid/common.hpp>
#include <mboxid/network.hpp>
#include <mboxid/error.hpp>
#include <mboxid/version.hpp>

namespace mboxid {

class backend_connector {
public:
    typedef std::uint64_t client_id;

    backend_connector() = default;
    backend_connector(const backend_connector&) = delete;
    backend_connector& operator=(const backend_connector&) = delete;
    backend_connector(backend_connector&&) = default;
    backend_connector& operator=(backend_connector&&) = default;
    virtual ~backend_connector() = default;

    virtual bool authorize([[maybe_unused]] client_id id,
                           [[maybe_unused]] const net::endpoint_addr&
                                   numeric_client_addr,
                           [[maybe_unused]] const sockaddr* addr,
                           [[maybe_unused]] socklen_t addrlen) {
        return true;
    }

    virtual void disconnect([[maybe_unused]] client_id id) {}

    virtual void alive([[maybe_unused]] client_id id) {}

    /**
     * Backend ticker is invoked approximately once a second.
     *
     * This method may be overridden to implement individual inactivity
     * timeouts for the clients, or to provide some kind of health monitoring.
     */
    virtual void ticker() { }

    virtual errc read_coils([[maybe_unused]] unsigned addr,
                            [[maybe_unused]] std::size_t cnt,
                            [[maybe_unused]] std::vector<bool>& bits) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc read_discrete_inputs([[maybe_unused]] unsigned addr,
                                      [[maybe_unused]] std::size_t cnt,
                            [[maybe_unused]] std::vector<bool>& bits) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc read_holding_registers([[maybe_unused]] unsigned addr,
                                        [[maybe_unused]] std::size_t cnt,
                                        [[maybe_unused]]
                                        std::vector<std::uint16_t>& regs) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc read_input_registers([[maybe_unused]] unsigned addr,
                                      [[maybe_unused]] std::size_t cnt,
                                      [[maybe_unused]]
                                      std::vector<std::uint16_t>& regs) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc write_coils([[maybe_unused]] unsigned addr,
                             [[maybe_unused]] const std::vector<bool>& bits) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc write_holding_registers([[maybe_unused]] unsigned addr,
                                         [[maybe_unused]]
                                         const std::vector<std::uint16_t>&
                                             regs) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc write_read_holding_registers([[maybe_unused]] unsigned addr_wr,
                                    [[maybe_unused]]
                                              const std::vector<std::uint16_t>&
                                                  regs_wr,
                                    [[maybe_unused]] unsigned addr_rd,
                                    [[maybe_unused]] std::size_t cnt_rd,
                                    [[maybe_unused]]
                                              std::vector<std::uint16_t>&
                                                  regs_rd) {
        return errc::modbus_exception_slave_or_server_failure;
    }

    virtual errc get_basic_device_identification(
                std::string& vendor, std::string& product, std::string& version)
    {
        vendor = get_vendor();
        product = get_product_name();
        version = get_version();
        return errc::none;
    }
};

} // namespace mboxid

#endif // LIBMBOXID_BACKEND_CONNECTOR_HPP
