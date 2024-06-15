// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Example of a Modbus TCP server application.
 */
#include <iostream>
#include <mboxid/modbus_tcp_client.hpp>

//! Connects to the server and queries the state of some coils.
void main_() {
    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
    auto coils = mb.read_coils(0, 3);
    for (size_t i = 0; i < coils.size(); ++i)
        std::cout << "coils[" << i << "]: " << coils[i] << "\n";
}

//! Wrapper around main_() that handles exceptions from the library.
int main() {
    try {
        main_();
        return 0;
    } catch (const mboxid::exception& e) {
        std::cerr << e.code() << ": " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
