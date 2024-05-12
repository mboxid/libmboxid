// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/error.hpp>
#include "modbus_protocol_common.hpp"

using namespace mboxid;
using testing::HasSubstr;

TEST(ModbusProtocolCommonTest, ParseMbapHeader) {
    mbap_header header; // NOLINT(*-pro-type-member-init)

    try {
        const uint8_t buf[]{0x01, 0x02, 0x00, 0x01};

        parse_mbap_header(buf, header);
    } catch (const mboxid_error& e) {
        EXPECT_THAT(e.what(), HasSubstr("incomplete"));
    }

    try {
        const uint8_t buf[]{0xca, 0xfe, 0, 1, 0, 2, 1};
        parse_mbap_header(buf, header);
    } catch (const mboxid_error& e) {
        EXPECT_THAT(e.what(), HasSubstr("protocol identifier invalid"));
    }

    try {
        const uint8_t buf[]{0xca, 0xfe, 0, 0, 0x00, 0x01, 1};
        parse_mbap_header(buf, header);
    } catch (const mboxid_error& e) {
        EXPECT_THAT(e.what(), HasSubstr("length field invalid"));
    }

    try {
        const uint8_t buf[]{0xca, 0xfe, 0, 0, 0x00, 255, 1};
        parse_mbap_header(buf, header);
    } catch (const mboxid_error& e) {
        EXPECT_THAT(e.what(), HasSubstr("length field invalid"));
    }

    const uint8_t buf[]{0xca, 0xfe, 0, 0, 0x00, 254, 1};
    parse_mbap_header(buf, header);
    EXPECT_EQ(header.transaction_id, 0xcafe);
    EXPECT_EQ(header.protocol_id, 0);
    EXPECT_EQ(header.length, 254);
    EXPECT_EQ(header.unit_id, 1);
}
TEST(ModbusProtocolCommonTest, BitToByteCound) {
    EXPECT_EQ(bit_to_byte_count(1), 1);
    EXPECT_EQ(bit_to_byte_count(8), 1);
    EXPECT_EQ(bit_to_byte_count(9), 2);
    EXPECT_EQ(bit_to_byte_count(16), 2);
    EXPECT_EQ(bit_to_byte_count(17), 3);
}