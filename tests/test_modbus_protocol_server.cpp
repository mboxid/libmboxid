// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/error.hpp>
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_server.hpp"

using namespace mboxid;
using testing::SetArgReferee;
using testing::Return;
using testing::DoAll;
using testing::Exactly;

using U8Vec = std::vector<uint8_t>;
using U16Vec = std::vector<uint16_t>;
using BoolVec = std::vector<bool>;

class BackendConnectorMock : public backend_connector {
public:
    using BoolVecRef = std::vector<bool>&;
    using BoolConstVecRef = const std::vector<bool>&;
    using U16VecRef = std::vector<uint16_t>&;
    using U16ConstVecRef = const std::vector<uint16_t>&;
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
};

TEST(ModbusProtocolServerTest, IllegalFunction) {
    U8Vec req{0x55, 0};
    U8Vec rsp_expected{0x55 | 0x80, 1};
    U8Vec rsp(max_pdu_size);
    BackendConnectorMock backend;

    auto cnt = server_engine(backend, req, rsp);
    EXPECT_EQ(cnt, rsp_expected.size());
    rsp.resize(cnt);
    EXPECT_EQ(rsp, rsp_expected);
}

TEST(ModbusProtocolServerTest, ReadCoils) {
    {
        // successful request
        BackendConnectorMock backend;
        BoolVec bits{
            // NOLINTNEXTLINE(*-use-bool-literals)
            1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1,
        };
        using testing::_;
        EXPECT_CALL(backend, read_coils(0x13, 0x13, _))
            .WillOnce(DoAll(SetArgReferee<2>(bits), Return
                            (errc::none)));

        U8Vec req{0x01, 0x00, 0x13, 0x00, 0x13};
        U8Vec rsp_expected{0x01, 0x03, 0xcd, 0x6b, 0x05};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong quantity --> illegal data value exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_coils).Times(Exactly(0));

        U8Vec req{0x01, 0x00, 0x13, 0x07, 0xd1};
        U8Vec rsp_expected{0x81, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_coils)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x01, 0x00, 0x13, 0x00, 0x13};
        U8Vec rsp_expected{0x81, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, ReadDiscreteInputs) {
    {
        // successful request
        BackendConnectorMock backend;
        BoolVec bits{
            // NOLINTNEXTLINE(*-use-bool-literals)
            0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1
        };
        using testing::_;
        EXPECT_CALL(backend, read_discrete_inputs(0xc4, 0x16, _))
            .WillOnce(DoAll(SetArgReferee<2>(bits),
                Return (errc::none)));

        U8Vec req{0x02, 0x00, 0xc4, 0x00, 0x16};
        U8Vec rsp_expected{0x02, 0x03, 0xac, 0xdb, 0x35};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong quantity --> illegal data value exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_discrete_inputs).Times(Exactly(0));

        U8Vec req{0x02, 0x00, 0xc4, 0x07, 0xd1};
        U8Vec rsp_expected{0x82, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_discrete_inputs)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x02, 0x00, 0xc4, 0x00, 0x16};
        U8Vec rsp_expected{0x82, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, ReadHoldingRegisters) {
    {
        // successful request
        BackendConnectorMock backend;
        U16Vec regs { 0x022b, 0x0000, 0x0064 };

        using testing::_;
        EXPECT_CALL(backend, read_holding_registers(0x6b, 0x03, _))
            .WillOnce(DoAll(SetArgReferee<2>(regs), Return (errc::none)));

        U8Vec req{0x03, 0x00, 0x6b, 0x00, 0x03};
        U8Vec rsp_expected{0x03, 0x06, 0x02, 0x2b, 0x00, 0x00, 0x00, 0x64};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong quantity --> illegal data value exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_holding_registers).Times(Exactly(0));

        U8Vec req{0x03, 0x00, 0x6b, 0x00, 0x7e};
        U8Vec rsp_expected{0x83, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_holding_registers)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x03, 0x00, 0x6b, 0x00, 0x03};
        U8Vec rsp_expected{0x83, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, ReadInputRegisters) {
    {
        // successful request
        BackendConnectorMock backend;
        U16Vec regs { 0x000a };

        using testing::_;
        EXPECT_CALL(backend, read_input_registers(0x08, 0x01, _))
            .WillOnce(DoAll(SetArgReferee<2>(regs), Return (errc::none)));

        U8Vec req{0x04, 0x00, 0x08, 0x00, 0x01};
        U8Vec rsp_expected{0x04, 0x02, 0x00, 0x0a};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong quantity --> illegal data value exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_input_registers).Times(Exactly(0));

        U8Vec req{0x04, 0x00, 0x08, 0x00, 0x7e};
        U8Vec rsp_expected{0x84, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, read_input_registers)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x04, 0x00, 0x08, 0x00, 0x01};
        U8Vec rsp_expected{0x84, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, WriteSingleCoil) {
    {
        // successful request
        BackendConnectorMock backend;
        BoolVec bits {1}; // NOLINT(*-use-bool-literals)

        EXPECT_CALL(backend, write_coils(0xac, bits))
            .WillOnce(Return (errc::none));

        U8Vec req{0x05, 0x00, 0xac, 0xff, 0x00};
        U8Vec rsp_expected{0x05, 0x00, 0xac, 0xff, 0x00};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong value --> illegal data value exception
        BackendConnectorMock backend;

        EXPECT_CALL(backend, write_coils).Times(Exactly(0));

        U8Vec req{0x05, 0x00, 0xac, 0xff, 0xff};
        U8Vec rsp_expected{0x85, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, write_coils)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x05, 0x00, 0xac, 0xff, 0x00};
        U8Vec rsp_expected{0x85, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, WriteSingleRegister) {
    {
        // successful request
        BackendConnectorMock backend;
        U16Vec regs {0x0003};

        EXPECT_CALL(backend, write_holding_registers(0x01, regs))
            .WillOnce(Return (errc::none));

        U8Vec req{0x06, 0x00, 0x01, 0x00, 0x03};
        U8Vec rsp_expected{0x06, 0x00, 0x01, 0x00, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, write_holding_registers)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x06, 0x00, 0x01, 0x00, 0x03};
        U8Vec rsp_expected{0x86, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, WriteMultipleCoils) {
    {
        // successful request
        BackendConnectorMock backend;
        BoolVec bits {
            1, 0, 1, 1, 0, 0, 1, 1, 1, 0 // NOLINT(*-use-bool-literals)
        };

        EXPECT_CALL(backend, write_coils(0x13, bits))
            .WillOnce(Return (errc::none));

        U8Vec req{0x0f, 0x00, 0x13, 0x00, 0x0a, 0x02, 0xcd, 0x01};
        U8Vec rsp_expected{0x0f, 0x00, 0x13, 0x00, 0x0a};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong value --> illegal data value exception
        BackendConnectorMock backend;

        EXPECT_CALL(backend, write_coils).Times(Exactly(0));

        U8Vec req{0x0f, 0x00, 0x13, 0x07, 0xb1, 0x02, 0xcd, 0x01};
        U8Vec rsp_expected{0x8f, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, write_coils)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x0f, 0x00, 0x13, 0x00, 0x0a, 0x02, 0xcd, 0x01};
        U8Vec rsp_expected{0x8f, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, WriteMultipleRegisters) {
    {
        // successful request
        BackendConnectorMock backend;
        U16Vec regs { 0x000a, 0x0102 };

        EXPECT_CALL(backend, write_holding_registers(0x01, regs))
            .WillOnce(Return (errc::none));

        U8Vec req{0x10, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x0a, 0x01, 0x02};
        U8Vec rsp_expected{0x10, 0x00, 0x01, 0x00, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong value --> illegal data value exception
        BackendConnectorMock backend;

        EXPECT_CALL(backend, write_holding_registers).Times(Exactly(0));

        U8Vec req{0x10, 0x00, 0x01, 0x00, 0x7c, 0xf8, 0x00, 0x0a, 0x01, 0x02};
        U8Vec rsp_expected{0x90, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, write_holding_registers)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x10, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x0a, 0x01, 0x02};
        U8Vec rsp_expected{0x90, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, MaskWriteRegister) {
    // successful request
    BackendConnectorMock backend;
    U16Vec regs_rd { 0x12 };
    U16Vec regs_wr { 0x17 };

    using testing::_;
    EXPECT_CALL(backend, read_holding_registers(0x04, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(regs_rd), Return(errc::none)));
    EXPECT_CALL(backend, write_holding_registers(0x04, regs_wr))
        .WillOnce(Return (errc::none));

    U8Vec req {0x16, 0x00, 0x04, 0x00, 0xf2, 0x00, 0x25};
    U8Vec rsp_expected {0x16, 0x00, 0x04, 0x00, 0xf2, 0x00, 0x25};
    U8Vec rsp(max_pdu_size);

    auto cnt = server_engine(backend, req, rsp);
    EXPECT_EQ(cnt, rsp_expected.size());
    rsp.resize(cnt);
    EXPECT_EQ(rsp, rsp_expected);
}

TEST(ModbusProtocolServerTest, ReadWriteMultipleRegisters) {
    {
        // successful request
        BackendConnectorMock backend;
        U16Vec regs_wr { 0x00ff, 0x00ff, 0x0ff };
        U16Vec regs_rd { 0x00fe, 0x0acd, 0x0001, 0x0003, 0x000d, 0x00ff };

        using testing::_;
        EXPECT_CALL(backend,
                    write_read_holding_registers(0x0e, regs_wr, 0x03, 0x06, _))
            .WillOnce(DoAll(SetArgReferee<4>(regs_rd), Return (errc::none)));

        U8Vec req{0x17, 0x00, 0x03, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x03, 0x06,
                0x00, 0xff, 0x00, 0xff, 0x00, 0xff};
        U8Vec rsp_expected{0x17, 0x0c, 0x00, 0xfe, 0x0a, 0xcd, 0x00, 0x01, 0x00,
                0x03, 0x00, 0x0d, 0x00, 0xff};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // write count too large  --> illegal data value exception
        BackendConnectorMock backend;

        EXPECT_CALL(backend, write_read_holding_registers).Times(Exactly(0));

        U8Vec req{0x17, 0x00, 0x00, 0x00, 0x7d, 0x00,
                  0x00, 0x00, 0x79, 0xf4, 0x00, 0x00};
        U8Vec rsp_expected{0x97, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // read count too large  --> illegal data value exception
        BackendConnectorMock backend;

        EXPECT_CALL(backend, write_read_holding_registers).Times(Exactly(0));

        U8Vec req{0x17, 0x00, 0x00, 0x00, 0x7e, 0x00,
                  0x00, 0x00, 0x01, 0x02, 0x00, 0x00};
        U8Vec rsp_expected{0x97, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong address --> illegal data address exception
        BackendConnectorMock backend;
        EXPECT_CALL(backend, write_read_holding_registers)
            .WillOnce(Return(errc::modbus_exception_illegal_data_address));

        U8Vec req{0x17, 0x00, 0x00, 0x00, 0x01, 0x00,
                  0x00, 0x00, 0x01, 0x02, 0x00, 0x00};
        U8Vec rsp_expected{0x97, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
}

TEST(ModbusProtocolServerTest, ReadDeviceIdentification) {
    {
        // successful request
        mboxid::backend_connector backend;
        U8Vec req {0x2b, 0x0e, 0x01, 0x00};
        U8Vec rsp_expected{0x2b, 0x0e, 0x01, 0x01, 0x00, 0x00, 0x03};
        U8Vec rsp(max_pdu_size);

        std::string vendor, product, version;
        vendor = get_vendor();
        product = get_product_name();
        version = get_version();

        rsp_expected.push_back(0x00);
        rsp_expected.push_back(vendor.length());
        rsp_expected.insert(rsp_expected.end(), vendor.begin(), vendor.end());

        rsp_expected.push_back(0x01);
        rsp_expected.push_back(product.length());
        rsp_expected.insert(rsp_expected.end(), product.begin(), product.end());

        rsp_expected.push_back(0x02);
        rsp_expected.push_back(version.length());
        rsp_expected.insert(rsp_expected.end(), version.begin(), version.end());

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong object id --> illegal data address exception
        mboxid::backend_connector backend;
        U8Vec req {0x2b, 0x0e, 0x01, 0xff};
        U8Vec rsp_expected{0x80 | 0x2b, 0x02};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
    {
        // wrong read device id code --> illegal data value exception
        mboxid::backend_connector backend;
        U8Vec req {0x2b, 0x0e, 0x0f, 0x00};
        U8Vec rsp_expected{0x80 | 0x2b, 0x03};
        U8Vec rsp(max_pdu_size);

        auto cnt = server_engine(backend, req, rsp);
        EXPECT_EQ(cnt, rsp_expected.size());
        rsp.resize(cnt);
        EXPECT_EQ(rsp, rsp_expected);
    }
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