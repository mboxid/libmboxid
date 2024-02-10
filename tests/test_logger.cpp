// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/error.hpp>
#include "logger_impl.hpp"

using namespace mboxid;

class LoggerMock : public logger_base {
public:
    MOCK_METHOD(void, debug, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, info, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, warning, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, error, (std::string_view  msg), (const, override));
    MOCK_METHOD(void, auth, (std::string_view  msg), (const, override));
};

TEST(LoggerTest, InstallInvalidLogger) {
    EXPECT_THROW(install_logger(std::unique_ptr<logger_base>()),
        general_error);
}

TEST(LoggerTest, UseLogger) {
    auto logger = std::make_unique<LoggerMock>();
    const LoggerMock* mock = logger.get();
    install_logger(std::move(logger));

    EXPECT_CALL(*mock, debug("debug 3.14")).Times(1);
    EXPECT_CALL(*mock, info("info 3.15")).Times(1);
    EXPECT_CALL(*mock, warning("warning 3.16")).Times(1);
    EXPECT_CALL(*mock, error(("error 3.17"))).Times(1);
    EXPECT_CALL(*mock, auth("auth 3.18")).Times(1);

    log_debug("debug {}.{}", 3, 14);
    log_info("info {}", 3.15);
    log_warning("warning {}.{}", 3, 16);
    log_error("error {}", "3.17");
    log_auth("auth {}.{}", 3, 18);

    // Revert to default logger to avoid the following error when the
    // test is executed:
    //      ERROR: this mock object (used in test LoggerTest.Main) should be
    //      deleted but never is.
    install_logger(make_standard_logger());
}
