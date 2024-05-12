// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/modbus_tcp_server.hpp>
#include <mboxid/modbus_tcp_client.hpp>
#include "modbus_protocol_common.hpp"
#include "network_private.hpp"
#include "logger_private.hpp"

using namespace mboxid;
using namespace std::chrono_literals;

using testing::_;
using testing::InSequence;
using testing::SetArgReferee;
using testing::Return;
using testing::DoAll;
using testing::HasSubstr;
using testing::NiceMock;

using U16Vec = std::vector<uint16_t>;
using BoolVec = std::vector<bool>;

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
        auto logger_ = std::make_unique<NiceMock<LoggerMock>>();
        install_logger(std::move(logger_));
        logger = dynamic_cast<const LoggerMock*>(log::borrow_logger());

        auto endpoints =
            resolve_endpoint("localhost", "1502", net::ip_protocol_version::v4,
                             net::endpoint_usage::passive_open);
        auto& ep = endpoints.front();

        listenfd = socket(ep.family, ep.socktype, ep.protocol);
        EXPECT_NE(listenfd, -1);

        int on = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
            throw system_error(errno, "setsockopt SO_REUSEADDR");

        int res;
        res = bind(listenfd, ep.addr.get(), ep.addrlen);
        EXPECT_EQ(res, 0);
        res = listen(listenfd, 1);
        EXPECT_EQ(res, 0);
    }

    void TearDown() override {
        stop_listen();

        // Revert to default logger to avoid the following error when the
        // test is executed:
        //      ERROR: this mock object (used in test LoggerTest.Main) should be
        //      deleted but never is.
        install_logger(log::make_standard_logger());
    }

    void accept_client() {
        connfd = accept(listenfd, nullptr, nullptr);
        EXPECT_NE(connfd, -1);
    }

    void stop_listen() {
        if (listenfd != -1) {
            close(listenfd);
            listenfd = -1;
        }
    }

    void close_connection() {
        if (connfd != -1) {
            close(connfd);
            connfd = -1;
        }
    }

    std::jthread server_thd;
    const LoggerMock *logger; // owned and freed by libmboxid
    int listenfd = -1;
    int connfd = -1;
};

TEST_F(ModbusTcpClientErrorHandlingTest, ConnectRefused) {
    stop_listen();

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
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] =
            {0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x01, 0x00};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    mb.read_coils(0, 1);
}

TEST_F(ModbusTcpClientErrorHandlingTest, WrongTransactionId) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] =
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x01, 0x00};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::parse_error);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, WrongUnitId) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] =
            {0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x01, 0x00};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
    try {
        mb.set_unit_id(5);
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::parse_error);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, ModbusException) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] =
            {0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x81, 0x02};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::modbus_exception_illegal_data_address);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, InvalidModbusExeptionCode) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] =
            {0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x81, 0x00};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::parse_error);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, InvalidModbusExceptionFunction) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] =
            {0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x82, 0x01};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;

    mb.connect_to_server("localhost", "1502");
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::parse_error);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, NoResponse) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    mb.set_response_timeout(1000ms);
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::timeout);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, IncompleteResponseHeader) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] = {0x00};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    mb.set_response_timeout(1000ms);
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::timeout);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, IncompleteResponseBody) {
    server_thd = std::jthread([&](){
        accept_client();
        uint8_t rsp[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01};
        write(connfd, rsp, sizeof(rsp));
    });

    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    mb.set_response_timeout(1000ms);
    try {
        mb.read_coils(0, 1);
        FAIL();
    }
    catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::timeout);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, PrematureClose) {
    server_thd = std::jthread([&]() {
        accept_client();
        uint8_t rsp[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01};
        write(connfd, rsp, sizeof(rsp));
        close_connection();
    });

    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    mb.set_response_timeout(1000ms);
    try {
        mb.read_coils(0, 1);
        FAIL();
    } catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::connection_closed);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, NotConnected) {
    mboxid::modbus_tcp_client mb;
    mb.set_response_timeout(1000ms);

    // initially not connected
    try {
        mb.read_coils(0, 1);
        FAIL();
    } catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::not_connected);
    }

    // connection closed by peer
    server_thd = std::jthread([&]() {
        accept_client();
        close_connection();
    });

    mb.connect_to_server("localhost", "1502");
    try {
        mb.read_coils(0, 1);
        FAIL();
    } catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::connection_closed);
    }

    // not connected after connection was closed by peer
    try {
        mb.read_coils(0, 1);
        FAIL();
    } catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::not_connected);
    }
}

TEST_F(ModbusTcpClientErrorHandlingTest, Disconnected) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    mb.disconnect();
    // not connected after client disconnected from server
    try {
        mb.read_coils(0, 1);
        FAIL();
    } catch (const mboxid_error& e) {
        EXPECT_EQ(e.code(), errc::not_connected);
    }
}

class BackendConnectorMock : public backend_connector {
public:
    using BoolVecRef = std::vector<bool>&;
    using BoolConstVecRef = const std::vector<bool>&;
    using U16VecRef = std::vector<uint16_t>&;
    using U16ConstVecRef = const std::vector<uint16_t>&;
    using StrRef = std::string&;

    MOCK_METHOD(errc, read_coils, (unsigned addr, std::size_t cnt,
        BoolVecRef bits), (override));
    MOCK_METHOD(errc, read_discrete_inputs, (unsigned addr, std::size_t cnt,
        BoolVecRef bits), (override));

    MOCK_METHOD(errc, read_holding_registers, (unsigned addr, std::size_t cnt,
        U16VecRef regs), (override));

    MOCK_METHOD(errc, read_input_registers, (unsigned addr, std::size_t cnt,
        U16VecRef regs), (override));

    MOCK_METHOD(errc, write_coils, (unsigned addr, BoolConstVecRef bits),
                (override));

    MOCK_METHOD(errc, write_holding_registers, (unsigned addr,
        U16ConstVecRef regs),
                (override));
    MOCK_METHOD(errc, write_read_holding_registers,
                (unsigned addr_wr, U16ConstVecRef regs_wr,
                    unsigned addr_rd, std::size_t cnt_rd, U16VecRef regs_rd),
                (override));
    MOCK_METHOD(errc, get_basic_device_identification,
                (StrRef vendor, StrRef product, StrRef version), (override));
};

class ModbusTcpClientAgainstServerTest : public testing::Test {
protected:
    ModbusTcpClientAgainstServerTest() {
        using namespace std::chrono_literals;

        server = std::make_unique<modbus_tcp_server>();
        server->set_server_addr("localhost", "1502");
        auto backend_ = std::make_unique<NiceMock<BackendConnectorMock>>();
        server->set_backend(std::move(backend_));
        backend = dynamic_cast<BackendConnectorMock*>(server->borrow_backend());
        server_run_thd = std::jthread(&modbus_tcp_server::run, &*server);

        // give server time to complete passive open
        usleep(100000);
    }

    ~ModbusTcpClientAgainstServerTest() {
        server->shutdown();
    }

    BackendConnectorMock* backend;  // owned and freed by the server
    std::unique_ptr<modbus_tcp_server> server;
    std::jthread server_run_thd;
};

TEST_F(ModbusTcpClientAgainstServerTest, ReadCoils) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    BoolVec bits{ 1, 0, 1 };
    EXPECT_CALL(*backend, read_coils(0xcafe, 3, _))
        .WillOnce(DoAll(SetArgReferee<2>(bits), Return(errc::none)));

    BoolVec bits_rsp = mb.read_coils(0xcafe, 3);
    EXPECT_EQ(bits_rsp, bits);
}

TEST_F(ModbusTcpClientAgainstServerTest, ReadDiscreteInpututs) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    BoolVec bits{ 1, 0, 1 };
    EXPECT_CALL(*backend, read_discrete_inputs(0xcafe, 3, _))
        .WillOnce(DoAll(SetArgReferee<2>(bits), Return(errc::none)));

    BoolVec bits_rsp = mb.read_discrete_inputs(0xcafe, 3);
    EXPECT_EQ(bits_rsp, bits);
}

TEST_F(ModbusTcpClientAgainstServerTest, ReadHoldingRegisters) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    U16Vec regs{ 1, 2, 3 };
    EXPECT_CALL(*backend, read_holding_registers(0xcafe, 3, _))
        .WillOnce(DoAll(SetArgReferee<2>(regs), Return(errc::none)));

    U16Vec regs_rsp = mb.read_holding_registers(0xcafe, 3);
    EXPECT_EQ(regs_rsp, regs);
}

TEST_F(ModbusTcpClientAgainstServerTest, ReadInputRegisters) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");
    U16Vec regs{ 1, 2, 3 };
    EXPECT_CALL(*backend, read_input_registers(0xcafe, 3, _))
        .WillOnce(DoAll(SetArgReferee<2>(regs), Return(errc::none)));

    U16Vec regs_rsp = mb.read_input_registers(0xcafe, 3);
    EXPECT_EQ(regs_rsp, regs);
}

TEST_F(ModbusTcpClientAgainstServerTest, WriteSingleCoil) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    {
        InSequence seq;

        EXPECT_CALL(*backend, write_coils(0xcafe, BoolVec{true}));
        EXPECT_CALL(*backend, write_coils(0xcafe, BoolVec{false}));
    }

    mb.write_single_coil(0xcafe, true);
    mb.write_single_coil(0xcafe, false);
}

TEST_F(ModbusTcpClientAgainstServerTest, WriteSingleRegister) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    EXPECT_CALL(*backend, write_holding_registers(0xcafe, U16Vec{0x4711}));

    mb.write_single_register(0xcafe, 0x4711);
}

TEST_F(ModbusTcpClientAgainstServerTest, WriteMultipleCoils) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    BoolVec bits{0, 1, 0};
    EXPECT_CALL(*backend, write_coils(0xcafe, bits));

    mb.write_multiple_coils(0xcafe, bits);
}

TEST_F(ModbusTcpClientAgainstServerTest, WriteMultipleRegisters) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    U16Vec regs{0x4711, 0xaffe, 0xc001};
    EXPECT_CALL(*backend, write_holding_registers(0xcafe, regs));

    mb.write_multiple_registers(0xcafe, regs);
}

TEST_F(ModbusTcpClientAgainstServerTest, MaskWriteRegister) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    {
        InSequence seq;

        EXPECT_CALL(*backend, read_holding_registers(0xcafe, 1, _))
            .WillOnce(DoAll(SetArgReferee<2>(U16Vec{0x12}),
                            Return(errc::none)));
        EXPECT_CALL(*backend, write_holding_registers(0xcafe, U16Vec{0x17}));
    }

    mb.mask_write_register(0xcafe, 0xf2, 0x25);
}

TEST_F(ModbusTcpClientAgainstServerTest, ReadWriteMultipleRegisters) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    U16Vec regs_wr{0x4711, 0xaffe, 0xc001};
    U16Vec regs_rd{0x4711, 0xaffe, 0xc001, 0xc0de};
    EXPECT_CALL(*backend,
                write_read_holding_registers(0xcafe, regs_wr, 0x0815, 4, _))
        .WillOnce(DoAll(SetArgReferee<4>(regs_rd), Return(errc::none)));

    auto regs = mb.read_write_multiple_registers(0xcafe, regs_wr, 0x0815, 4);
    EXPECT_EQ(regs, regs_rd);
}

TEST_F(ModbusTcpClientAgainstServerTest, ReadDeviceIdentification) {
    mboxid::modbus_tcp_client mb;
    mb.connect_to_server("localhost", "1502");

    U16Vec regs_wr{0x4711, 0xaffe, 0xc001};
    U16Vec regs_rd{0x4711, 0xaffe, 0xc001, 0xc0de};
    std::string vendor{"vendor"};
    std::string product{"product"};
    std::string version{"1.0"};
    EXPECT_CALL(*backend,
                get_basic_device_identification(_, _, _))
        .WillOnce(DoAll(SetArgReferee<0>(vendor),
                        SetArgReferee<1>(product),
                        SetArgReferee<2>(version), Return(errc::none)));

    std::string vendor_rsp;
    std::string product_rsp;
    std::string version_rsp;
    mb.read_device_identification(vendor_rsp, product_rsp, version_rsp);
    EXPECT_EQ(vendor_rsp, vendor);
    EXPECT_EQ(product_rsp, product);
    EXPECT_EQ(version_rsp, version);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    GTEST_FLAG_SET(catch_exceptions, 0);
    try {
        return RUN_ALL_TESTS();
    }
    catch (mboxid_error& e) {
        std::cerr << e.code() << ": " << e.what() << "\n";
    }
    return EXIT_FAILURE;
}
