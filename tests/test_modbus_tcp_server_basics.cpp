// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <future>
#include <gtest/gtest.h>
#include <mboxid/modbus_tcp_server.hpp>

using namespace mboxid;

TEST(ModbusTcpServerBasics, RequestStop) {
    using namespace std::chrono_literals;
    modbus_tcp_server server;
    server.set_server_addr("localhost", "1502");
    auto f = std::async(std::launch::async, &modbus_tcp_server::run, &server);
    server.shutdown();

    EXPECT_EQ(f.wait_for(1s), std::future_status::ready)
        << "failed to stop server";
    (void) f.get(); // check for exception thrown by the server
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    GTEST_FLAG_SET(break_on_failure, 1);
    return RUN_ALL_TESTS();
}