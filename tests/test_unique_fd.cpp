#include <gtest/gtest.h>
#include <errno.h>
#include "unique_fd.hpp"

using namespace mboxid;

TEST(UniqueFdTest, Main) {
    int fds[2];

    ASSERT_EQ(pipe((fds)), 0);

    // test constructor which gets the file descriptor passed to it
    Unique_fd fd1(fds[0]);
    EXPECT_EQ(fd1.get(), fds[0]);

    // test default constructor
    Unique_fd fd2;
    EXPECT_EQ(fd2.get(), -1);
    // pass file descriptor to the container
    fd2.reset(fds[1]);
    EXPECT_EQ(fd2.get(), fds[1]);

    {
        // test move constructor
        Unique_fd fd3{std::move(fd2)};
        EXPECT_EQ(fd2.get(), -1);
        EXPECT_EQ(fd3.get(), fds[1]);

        // test move assignment operator
        Unique_fd fd4;
        fd4 = std::move(fd3);
        EXPECT_EQ(fd4.get(), fds[1]);
    }

    // test that the destructor from fd4 has closed the file
    errno = 0;
    EXPECT_EQ(close(fds[1]), -1);
    EXPECT_EQ(errno, EBADF);

    // test release() method
    int fd = fd1.release();
    EXPECT_EQ(fd1.get(), -1);
    EXPECT_EQ(close(fd), 0);
}