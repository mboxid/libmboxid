// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cerrno>
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

TEST(ExceptionsTest, GeneralError) {
    mboxid_error err(errc::invalid_argument, "hugo");

    EXPECT_EQ(err.code().category(), mboxid_category());
    EXPECT_EQ(err.code().value(),
              static_cast<int>(errc::invalid_argument));
    EXPECT_THAT(err.what(), HasSubstr("hugo"));
    EXPECT_THAT(err.what(), HasSubstr("invalid argument"));
}

TEST(ErrorCodeTest, Success) {
    auto ec = make_error_code(errc::none);

    EXPECT_EQ(ec, errc());
}

TEST(ExceptionTest, ModbusException) {
    {
        system_error err(static_cast<int>(
                            errc::modbus_exception_illegal_function));
        EXPECT_FALSE(is_modbus_exception(err));
    }
    {
        mboxid_error err(errc::none);
        EXPECT_FALSE(is_modbus_exception(err));
    }
    {
        mboxid_error err(errc::invalid_argument);
        EXPECT_FALSE(is_modbus_exception(err));
    }
    {
        mboxid_error err(errc::modbus_exception_illegal_function);
        EXPECT_FALSE(is_modbus_exception(err));
    }
    {
        mboxid_error err(errc::modbus_exception_gateway_target);
        EXPECT_TRUE(is_modbus_exception(err));
    }
}
