// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Example of a Modbus TCP server application.
 */
#include <cstdlib>
#include <csignal>
#include <system_error>
#include <iostream>
#include <thread>
#include <atomic>
#include <initializer_list>
#include <mboxid/modbus_tcp_server.hpp>

static std::atomic<int> exit_code = EXIT_SUCCESS;

static void block_signals() {
    sigset_t set;

    if (auto err = sigfillset(&set))
        throw std::system_error(err, std::system_category(), "sigfillset");

    if (auto err = pthread_sigmask(SIG_BLOCK, &set, nullptr))
        throw std::system_error(err, std::system_category(), "pthread_sigmask");
}

static int wait_signal(std::initializer_list<int> signal_list) {
    sigset_t set;

    if (auto err = sigemptyset(&set))
        throw std::system_error(err, std::system_category(), "sigempty");

    for (auto&& sig : signal_list) {
        if (auto err = sigaddset(&set, sig))
            throw std::system_error(err, std::system_category(), "sigaddset");
    }

    int sig;
    if (auto err = sigwait(&set, &sig))
        throw std::system_error(err, std::system_category(), "sigwait");

    return sig;
}

//! Connects the server with the user application.
class backend_connector : public mboxid::backend_connector {
public:
    mboxid::errc read_coils(
            unsigned addr, std::size_t cnt, std::vector<bool>& bits) override;
    mboxid::errc read_discrete_inputs(
            unsigned addr, std::size_t cnt, std::vector<bool>& bits) override;
    mboxid::errc read_holding_registers(unsigned addr, std::size_t cnt,
            std::vector<std::uint16_t>& regs) override;
    mboxid::errc read_input_registers(unsigned addr, std::size_t cnt,
            std::vector<std::uint16_t>& regs) override;
    mboxid::errc write_coils(
            unsigned addr, const std::vector<bool>& bits) override;
    mboxid::errc write_holding_registers(
            unsigned addr, const std::vector<std::uint16_t>& regs) override;
    mboxid::errc write_read_holding_registers(unsigned addr_wr,
            const std::vector<std::uint16_t>& regs_wr, unsigned addr_rd,
            std::size_t cnt_rd, std::vector<std::uint16_t>& regs_rd) override;

private:
    // Exemplary application data.
    std::vector<bool> coils{false, false, false, false, false, false, false,
            false, false, false};
    std::vector<bool> discrete_inputs{
            false, true, true, false, false, false, false, false, false, true};
    std::vector<uint16_t> input_registers{0, 1, 2, 3, 4};
    std::vector<uint16_t> holding_registers{0, 0, 0, 0, 0};
};

mboxid::errc backend_connector::read_coils(
        unsigned int addr, std::size_t cnt, std::vector<bool>& bits) {
    if (!cnt || (cnt > coils.size()) || ((addr + cnt) > coils.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (std::size_t i = 0; i < cnt; ++i)
        bits.push_back(coils[addr + i]);

    return mboxid::errc::none;
}

mboxid::errc backend_connector::read_discrete_inputs(
        unsigned int addr, std::size_t cnt, std::vector<bool>& bits) {
    if (!cnt || (cnt > discrete_inputs.size()) ||
            ((addr + cnt) > discrete_inputs.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (std::size_t i = 0; i < cnt; ++i)
        bits.push_back(discrete_inputs[addr + i]);

    return mboxid::errc::none;
}

mboxid::errc backend_connector::read_input_registers(
        unsigned int addr, std::size_t cnt, std::vector<std::uint16_t>& regs) {
    if (!cnt || (cnt > input_registers.size()) ||
            ((addr + cnt) > input_registers.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (std::size_t i = 0; i < cnt; ++i)
        regs.push_back(input_registers[addr + i]);

    return mboxid::errc::none;
}

mboxid::errc backend_connector::read_holding_registers(
        unsigned int addr, std::size_t cnt, std::vector<std::uint16_t>& regs) {
    if (!cnt || (cnt > holding_registers.size()) ||
            ((addr + cnt) > holding_registers.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (std::size_t i = 0; i < cnt; ++i)
        regs.push_back(holding_registers[addr + i]);

    return mboxid::errc::none;
}

mboxid::errc backend_connector::write_coils(
        unsigned addr, const std::vector<bool>& bits) {
    if (bits.empty() || ((addr + bits.size()) > coils.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (auto b : bits)
        coils[addr++] = b;

    return mboxid::errc::none;
}

mboxid::errc backend_connector::write_holding_registers(
        unsigned addr, const std::vector<std::uint16_t>& regs) {
    if (regs.empty() || ((addr + regs.size()) > holding_registers.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (auto val : regs)
        holding_registers[addr++] = val;

    return mboxid::errc::none;
}

mboxid::errc backend_connector::write_read_holding_registers(
        unsigned int addr_wr, const std::vector<std::uint16_t>& regs_wr,
        unsigned int addr_rd, std::size_t cnt_rd,
        std::vector<std::uint16_t>& regs_rd) {

    if (regs_wr.empty() ||
            ((addr_wr + regs_wr.size()) > holding_registers.size()) ||
            ((addr_rd + cnt_rd) > holding_registers.size()))
        return mboxid::errc::modbus_exception_illegal_data_address;

    for (auto val : regs_wr)
        holding_registers[addr_wr++] = val;

    for (std::size_t i = 0; i < cnt_rd; ++i)
        regs_rd.push_back(holding_registers[addr_rd + i]);

    return mboxid::errc::none;
}

/*!
 * Entry point for a separate thread which runs the Modbus server.
 * @param server Smart pointer to an instance of the server class.
 *
 * \note
 * Clang-Tidy complains:
 *      The parameter 'server' is copied for each invocation but only used as a
 *      const reference; consider making it a const reference
 * As 'server' is a shared pointer with reference counting, we consider this
 * warning as false positive.
 */
// NOLINTNEXTLINE(*-unnecessary-value-param)
static void server_thread(std::shared_ptr<mboxid::modbus_tcp_server> server) {
    try {
        server->set_server_addr("localhost", "1502");
        server->set_backend(std::make_unique<backend_connector>());
        server->run();
    } catch (const mboxid::exception& e) {
        std::cerr << e.code() << ": " << e.what() << "\n";
        exit_code = EXIT_FAILURE;
        kill(getpid(), SIGTERM);
    }
}

/*!
 * Manages the life cycle of the server.
 *
 * The main function manages the life cycle of the server. It starts the
 * server in a separate thread, and blocks till the user requests the
 * process to terminate (SIGTERM). Thereafter, it tells the server to shut
 * shut down and waits until the server thread has finished.
 */
int main() {
    block_signals();

    auto server = std::make_shared<mboxid::modbus_tcp_server>();
    auto server_thd = std::thread(server_thread, server);

    (void)wait_signal({SIGINT, SIGTERM});
    server->shutdown();
    server_thd.join();

    return exit_code;
}
