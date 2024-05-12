#include <gtest/gtest.h>
#include <mboxid/common.hpp>
#include "byteorder.hpp"

using namespace mboxid;

TEST(ByteorderTest, Fetch8) {
    uint8_t buf[] = {0xca};
    unsigned v;
    EXPECT_EQ(fetch8(v, buf), 1);
    EXPECT_EQ(v, 0xca);
}

TEST(ByteorderTest, Fetch16BE) {
    uint8_t buf[] = {0xca, 0xfe};
    unsigned v;
    EXPECT_EQ(fetch16_be(v, buf), 2);
    EXPECT_EQ(v, 0xcafe);
}

TEST(ByteorderTest, Store8) {
    uint8_t buf[1];
    EXPECT_EQ(store8(buf, 0xca), 1);
    EXPECT_EQ(buf[0], 0xca);
}

TEST(ByteorderTest, Store16Be) {
    uint8_t buf[2];
    EXPECT_EQ(store16_be(buf, 0xaffe), 2);
    EXPECT_EQ(buf[0], 0xaf);
    EXPECT_EQ(buf[1], 0xfe);
}
