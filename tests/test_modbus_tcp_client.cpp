// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/modbus_tcp_client.hpp>
#include "modbus_protocol_common.hpp"
#include "network_private.hpp"
#include "logger_private.hpp"

using namespace mboxid;
using namespace std::chrono_literals;

using testing::HasSubstr;
using testing::NiceMock;

class LoggerMock : public log::logger_base {
public:
    MOCK_METHOD(void, debug, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, info, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, warning, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, error, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, auth, (std::string_view  msg), (const, override));
};

class ModbusTcpClientErrorHandlingTest : public testing::Test {
protected:
    void SetUp() override {
        auto logger = std::make_unique<NiceMock<LoggerMock>>();
        this->logger = logger.get();

        install_logger(std::move(logger));

        auto endpoints =
            resolve_endpoint("localhost", "1502", net::ip_protocol_version::v4,
                             net::endpoint_usage::passive_open);
        auto& ep = endpoints.front();

        fd = socket(ep.family, ep.socktype, ep.protocol);
        EXPECT_NE(fd, -1);

        int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
            throw system_error(errno, "setsockopt SO_REUSEADDR");

        int res;
        res = bind(fd, ep.addr.get(), ep.addrlen);
        EXPECT_EQ(res, 0);
        res = listen(fd, 1);
        EXPECT_EQ(res, 0);
    }

    void TearDown() override {
        close_socket();

        // Revert to default logger to avoid the following error when the
        // test is executed:
        //      ERROR: this mock object (used in test LoggerTest.Main) should be
        //      deleted but never is.
        install_logger(log::make_standard_logger());
    }

    void close_socket() {
        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }

    LoggerMock *logger;
    int fd = -1;
};

TEST_F(ModbusTcpClientErrorHandlingTest, ConnectRefused) {
    close_socket();

    mboxid::modbus_tcp_client mb;

    // we expect an error message for IPv4, and another for IPv6.
    EXPECT_CALL(*logger, error(HasSubstr("Connection refused"))).Times(2);
    EXPECT_THROW(mb.connect_to_server("localhost", "1502"), mboxid_error);
}

TEST_F(ModbusTcpClientErrorHandlingTest, ConnectTimeout) {
    EXPECT_CALL(*logger, error(HasSubstr("Connection timed out"))).Times(1);

    try {
        // Even though we don't accept the connection, there can be several
        // established connections in the backlog. Therefore, we need to
        // connect to the open port multiple times until the timeout is
        // triggered.
        std::vector<modbus_tcp_client> mbs(5);

        for (auto& mb : mbs) {
            mb.connect_to_server("localhost", "1502",
                                 net::ip_protocol_version::v4, 1000ms);
        }

        FAIL();
    }
    catch (mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::active_open_error);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, Faultless) {
    std::thread t([&](){
        auto connfd = accept(fd, nullptr, 0);
        EXPECT_NE(connfd, -1);
        uint8_t rsp[] =
            {0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00};
        write(fd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;
    mb.set_response_timeout(1000ms);

    mb.connect_to_server("localhost", "1502");
    mb.read_coils(0, 1);
    t.join();
}