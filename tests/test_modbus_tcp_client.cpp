// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <thread>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/modbus_tcp_client.hpp>
#include "modbus_protocol_common.hpp"
#include "network_private.hpp"
#include "unique_fd.hpp"
#include "logger_private.hpp"

using namespace mboxid;

using testing::HasSubstr;

class LoggerMock : public log::logger_base {
public:
    MOCK_METHOD(void, debug, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, info, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, warning, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, error, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, auth, (std::string_view  msg), (const, override));
};

class ModbusTcpClientConnectTest : public testing::Test {
protected:
    ModbusTcpClientConnectTest() {
        auto logger = std::make_unique<LoggerMock>();
        this->logger = logger.get();

        install_logger(std::move(logger));
    }

    ~ModbusTcpClientConnectTest() {
        // Revert to default logger to avoid the following error when the
        // test is executed:
        //      ERROR: this mock object (used in test LoggerTest.Main) should be
        //      deleted but never is.
        install_logger(log::make_standard_logger());
    }

    LoggerMock *logger;
};

TEST_F(ModbusTcpClientConnectTest, Refused) {
    mboxid::modbus_tcp_client mb;

    EXPECT_CALL(*logger, error(HasSubstr("Connection refused"))).Times(2);
    EXPECT_THROW(mb.connect_to_server("localhost", "1502"), mboxid_error);
}

TEST_F(ModbusTcpClientConnectTest, Timeout) {
    using namespace std::chrono_literals;

    auto endpoints =
        resolve_endpoint("localhost", "1502", net::ip_protocol_version::v4,
                         net::endpoint_usage::passive_open);
    auto& ep = endpoints.front();

    int fd = socket(ep.family, ep.socktype, ep.protocol);
    EXPECT_NE(fd, -1);

    unique_fd ufd(fd);

    int res;
    res = bind(fd, ep.addr.get(), ep.addrlen);
    EXPECT_EQ(res, 0);
    res = listen(fd, 1);
    EXPECT_EQ(res, 0);

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