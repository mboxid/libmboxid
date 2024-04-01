// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <memory>
#include <thread>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/modbus_tcp_server.hpp>

using namespace mboxid;

using testing::Exactly;
using testing::Between;

TEST(ModbusTcpServerBasicsTest, Shutdown) {
    using namespace std::chrono_literals;
    modbus_tcp_server server;
    server.set_server_addr("localhost", "1502");
    auto f = std::async(std::launch::async, &modbus_tcp_server::run, &server);
    server.shutdown();

    EXPECT_EQ(f.wait_for(1s), std::future_status::ready)
        << "failed to stop server";
    (void) f.get(); // check for exception thrown by the server
}

class BackendConnectorMock : public backend_connector
{
public:
    MOCK_METHOD(void, ticker, (), (override));
};

class ModbusTcpServerTest : public ::testing::Test {
protected:
    ModbusTcpServerTest() {
        server = std::make_unique<modbus_tcp_server>();
        server->set_server_addr("localhost", "1502");
        auto backend_ = std::make_unique<BackendConnectorMock>();
        backend = backend_.get();
        server->set_backend(std::move(backend_));
        server_run_thd = std::thread(&modbus_tcp_server::run, &*server);
        std::cerr << "server started\n";
    }

    ~ModbusTcpServerTest() {
        server->shutdown();
        server_run_thd.join();
        std::cerr << "server stopped\n";
    }

    BackendConnectorMock* backend;  // owned and freed by the server
    std::unique_ptr<modbus_tcp_server> server;
    std::thread server_run_thd;
};

TEST_F(ModbusTcpServerTest, Ticker) {
    EXPECT_CALL(*backend, ticker).Times(Between(1, 2));
    sleep(2);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    GTEST_FLAG_SET(break_on_failure, 1);
    return RUN_ALL_TESTS();
}