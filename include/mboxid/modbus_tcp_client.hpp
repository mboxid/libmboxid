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
    void disconnect();
    void set_response_timeout(milliseconds timeout);
    void set_unit_id(unsigned id);

    std::vector<bool> read_coils(unsigned addr, size_t cnt);
    std::vector<bool> read_discrete_inputs(unsigned addr, size_t cnt);
    std::vector<uint16_t> read_holding_registers(unsigned addr, size_t cnt);
    std::vector<uint16_t> read_input_registers(unsigned addr, size_t cnt);
    void write_single_coil(unsigned addr, bool on);
    void write_single_register(unsigned addr, unsigned val);
    void write_multiple_coils(unsigned addr, const std::vector<bool>& bits);
    void write_multiple_registers(unsigned addr,
                                  const std::vector<uint16_t>& regs);
    void mask_write_register(unsigned addr, unsigned and_msk, unsigned or_msk);
    std::vector<uint16_t> read_write_multiple_registers(
                                unsigned addr_wr,
                                const std::vector<uint16_t>& regs_wr,
                                unsigned addr_rd, size_t cnt_rd);
    void read_device_identification(std::string& vendor,
                                    std::string& product,
                                    std::string& version);

private:
    std::unique_ptr<context> ctx;
};

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_TCP_CLIENT_HPP
