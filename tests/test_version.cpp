// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mboxid/version.hpp>

using namespace mboxid;
using testing::HasSubstr;

TEST(VersionTest, Main)
{
    int major, minor, patch;
    get_version(major, minor, patch);
    auto version = get_version();
    auto verbose_version = get_verbose_version();

    EXPECT_STREQ(version,
                 (std::to_string(major) + "." + std::to_string(minor) + "." +
                 std::to_string(patch)).c_str());
    EXPECT_THAT(verbose_version, HasSubstr("libmboxid"));
    EXPECT_THAT(verbose_version, HasSubstr(version));
}