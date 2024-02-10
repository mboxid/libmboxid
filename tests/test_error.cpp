// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <errno.h>
#include <mboxid/error.hpp>

using namespace mboxid;
using testing::HasSubstr;

TEST(ExceptionsTest, SystemError) {
    system_error err(EINVAL, "hugo");

    EXPECT_EQ(err.code().category(), std::system_category());
    EXPECT_EQ(err.code().value(), EINVAL);
    EXPECT_THAT(err.what(), HasSubstr("hugo"));
    EXPECT_THAT(err.what(), HasSubstr("Invalid argument"));
}

TEST(ExceptionsTest, ModbusError) {
    modbus_error err(modbus_errc::illegal_function, "hugo");

    EXPECT_EQ(err.code().category(), modbus_category());
    EXPECT_EQ(err.code().value(),
              static_cast<int>(modbus_errc::illegal_function));
    EXPECT_THAT(err.what(), HasSubstr("hugo"));
    EXPECT_THAT(err.what(), HasSubstr("illegal function"));
}

TEST(ExceptionsTest, GeneralError) {
    general_error err(general_errc::work_in_progress, "hugo");

    EXPECT_EQ(err.code().category(), general_category());
    EXPECT_EQ(err.code().value(),
              static_cast<int>(general_errc::work_in_progress));
    EXPECT_THAT(err.what(), HasSubstr("hugo"));
    EXPECT_THAT(err.what(), HasSubstr("in progress"));
}
