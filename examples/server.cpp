// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <cstdlib>
#include <csignal>
#include <system_error>
#include <iostream>
#include <thread>
#include <atomic>
#include <mboxid/modbus_tcp_server.hpp>

static std::atomic<int> exit_code = EXIT_SUCCESS;

static void block_signals(sigset_t* set) {
    sigemptyset(set);
    sigaddset(set, SIGINT);
    sigaddset(set, SIGTERM);
    if (auto err = pthread_sigmask(SIG_BLOCK, set, nullptr)) {
        throw std::system_error(err, std::system_category(), "pthread_sigmask");
    }
}

static void wait_signal(const sigset_t* set) {
    int sig;
    if (auto err = sigwait(set, &sig))
        throw std::system_error(err, std::system_category(), "sigwait");
}

static void server_thread(std::shared_ptr<mboxid::modbus_tcp_server> server) {
    try {
        server->set_server_addr("localhost", "1502");
        server->run();
    }
    catch (const mboxid::exception& e) {
        std::cerr << e.code() << ": " << e.what() << "\n";
        exit_code = EXIT_FAILURE;
        kill(getpid(), SIGTERM);
    }
}

int main() {
    sigset_t blocked;
    block_signals(&blocked);

    auto server = std::make_shared<mboxid::modbus_tcp_server>();
    auto server_thd = std::thread(server_thread, server);

    wait_signal(&blocked);
    server->shutdown();
    server_thd.join();

    return exit_code;
}