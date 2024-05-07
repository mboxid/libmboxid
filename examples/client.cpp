// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>
#include <mboxid/modbus_tcp_client.hpp>

void main_() {
    using namespace std::chrono_literals;

    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
}

int main() {
    try {
        main_();
        return 0;
    }
    catch (const mboxid::exception& e) {
        std::cerr << e.code() << ": " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
