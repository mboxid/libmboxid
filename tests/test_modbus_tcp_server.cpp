// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <thread>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/modbus_tcp_server.hpp>
#include "modbus_protocol_common.hpp"
#include "network_private.hpp"

using namespace mboxid;

using testing::_;
using testing::Exactly;
using testing::Between;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;
using testing::AnyNumber;
using testing::NiceMock;

using u8vec = std::vector<uint8_t>;

TEST(ModbusTcpServerBasicTest, Shutdown) {
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

    MOCK_METHOD(bool, authorize, (client_id id,const net::endpoint_addr&
                                                    numeric_client_addr,
                                  const sockaddr* addr, socklen_t addrlen),
                (override));

    MOCK_METHOD(void, disconnect, (client_id id), (override));
    MOCK_METHOD(void, alive, (client_id id), (override));
};

class ModbusTcpServerTest : public ::testing::Test {
protected:
    ModbusTcpServerTest() {
        using namespace std::chrono_literals;

        server = std::make_unique<modbus_tcp_server>();
        server->set_server_addr("localhost", "1502");
        auto backend_ = std::make_unique<NiceMock<BackendConnectorMock>>();
        backend = backend_.get();
        server->set_backend(std::move(backend_));
        server->set_idle_timeout(1000ms);
        server->set_request_complete_timeout(100ms);
        server_run_thd = std::thread(&modbus_tcp_server::run, &*server);

        // give server time to start
        usleep(100000);
    }

    ~ModbusTcpServerTest() {
        server->shutdown();
        server_run_thd.join();
    }


    BackendConnectorMock* backend;  // owned and freed by the server
    std::unique_ptr<modbus_tcp_server> server;
    std::thread server_run_thd;
};

static int connect_to_server() {
    auto endpoints =
        resolve_endpoint("localhost", "1502", net::ip_protocol_version::v4,
                         net::endpoint_usage::active_open);
    auto& ep = endpoints.front();

    int fd;

    if ((fd = socket(ep.family, ep.socktype, ep.protocol)) == -1)
        return -1;

    if (connect(fd, ep.addr.get(), ep.addrlen) == -1)
        return -1;

    return fd;
}

// returns the number of bytes received, 0 for EOF, or -1 in case of an error.
static int receive_all(int fd, uint8_t* buf, size_t cnt) {
    size_t left = cnt;

    do {
        ssize_t res;
        res = TEMP_FAILURE_RETRY(read(fd, &buf[cnt - left], left));
        if (res <= 0)
            return res;
        left -= res;
    } while (left);
    return cnt;
}

TEST_F(ModbusTcpServerTest, Ticker) {
    EXPECT_CALL(*backend, ticker).Times(Between(1, 2));
    sleep(2);
}

TEST_F(ModbusTcpServerTest, RequestResponse) {
    using namespace std::chrono_literals;

    EXPECT_CALL(*backend, authorize).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*backend, disconnect).Times(1);
    EXPECT_CALL(*backend, alive).Times(1);

    int fd = connect_to_server();
    ASSERT_NE(fd, -1);

    u8vec req { 0x47, 0x11, 0x00, 0x00, 0x00, 0x06, 0xaa, 0x01, 0x00, 0x00,
              0x00, 0x01};
    u8vec rsp_expected { 0x47, 0x11, 0x00, 0x00, 0x00, 0x03, 0xaa, 0x81, 0x04 };
    u8vec rsp(rsp_expected.size());

    int res;

    res = TEMP_FAILURE_RETRY(write(fd, req.data(), req.size()));
    EXPECT_EQ(res, req.size());

    auto f = std::async(std::launch::async, receive_all, fd, rsp.data(),
                        rsp.size());

    EXPECT_EQ(f.wait_for(200ms), std::future_status::ready)
                << "server did not respond within the time limit";
    res = f.get(); // check for exception thrown by the server
    EXPECT_GT(res, 0);
    EXPECT_EQ(rsp, rsp_expected);

    close(fd);
    // give server time to close the connection
    usleep(100000);
}

TEST_F(ModbusTcpServerTest, CloseClientConnection) {
    using namespace std::chrono_literals;

    volatile modbus_tcp_server::client_id id = 0;
    EXPECT_CALL(*backend, authorize).Times(1)
        .WillOnce(DoAll(SaveArg<0>(&id), Return(true)));
    EXPECT_CALL(*backend, disconnect).Times(1);

    int fd = connect_to_server();
    ASSERT_NE(fd, -1);

    // wait till authorize() is called
    for (int i = 0; !id && (i < 10); ++i)
        usleep(100000);

    server->close_client_connection(id);

    u8vec rsp(max_pdu_size);

    auto f = std::async(std::launch::async, receive_all, fd, rsp.data(),
                        rsp.size());

    EXPECT_EQ(f.wait_for(500ms), std::future_status::ready)
                << "server did not respond within the time limit";
    int res = f.get(); // check for exception thrown by the server
    EXPECT_EQ(res, 0);

    close(fd);
}

TEST_F(ModbusTcpServerTest, IdleTimeout) {
    using namespace std::chrono_literals;

    EXPECT_CALL(*backend, authorize).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*backend, disconnect).Times(1);

    int fd = connect_to_server();
    ASSERT_NE(fd, -1);

    u8vec rsp(max_pdu_size);

    auto f = std::async(std::launch::async, receive_all, fd, rsp.data(),
                        rsp.size());

    EXPECT_EQ(f.wait_for(2000ms), std::future_status::ready)
                << "server did not respond within the time limit";
    int res = f.get(); // check for exception thrown by the server
    EXPECT_EQ(res, 0);

    close(fd);
}

TEST_F(ModbusTcpServerTest, RequestTimeout) {
    using namespace std::chrono_literals;

    EXPECT_CALL(*backend, authorize).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*backend, disconnect).Times(1);

    int fd = connect_to_server();
    ASSERT_NE(fd, -1);

    u8vec req { 0x47, 0x11, 0x00, 0x00, 0x00, 0x06, 0xaa, 0x01 };

    int res;

    res = TEMP_FAILURE_RETRY(write(fd, req.data(), req.size()));
    EXPECT_EQ(res, req.size());

    u8vec rsp(max_pdu_size);

    auto f = std::async(std::launch::async, receive_all, fd, rsp.data(),
                        rsp.size());

    EXPECT_EQ(f.wait_for(200ms), std::future_status::ready)
                << "server did not respond within the time limit";
    res = f.get(); // check for exception thrown by the server
    EXPECT_EQ(res, 0);

    close(fd);
}


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    GTEST_FLAG_SET(break_on_failure, 1);
    return RUN_ALL_TESTS();
}