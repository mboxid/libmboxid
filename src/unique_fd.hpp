// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_UNIQUE_FD_HPP
#define LIBMBOXID_UNIQUE_FD_HPP

#include <unistd.h>

namespace mboxid {

/**
 * Container for a file descriptor that closes the descriptor when it goes
 * out of scope.
 */
class unique_fd {
public:
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;

    unique_fd() : fd{-1} {}
    explicit unique_fd(int fd) : fd{fd} {}

    unique_fd(unique_fd&& other)  noexcept : fd{other.release()} {}

    unique_fd& operator=(unique_fd&& other)  noexcept {
        reset(other.release());
        return *this;
    }

    ~unique_fd() {
        reset();
    }

    [[nodiscard]] int get() const { return fd; }

    void reset(int new_fd = -1) {
        if (fd != -1) {
            /*
             * man 2 close:
             * > The EINTR error is a somewhat special case.  Regarding the
             * > EINTR error, POSIX.1-2008 says:
             * >
             * >    If close() is interrupted by a signal that is to be caught,
             * >    it shall return -1 with errno set to EINTR and the state of
             * >    fildes is unspecified.
             * >
             * > This permits the behavior that occurs on Linux and many other
             * > implementations, where, as with other errors that may be
             * > reported by close(), the file descriptor is guaranteed to be
             * > closed.
             */
            (void) close(fd);
        }
        fd = new_fd;
    }

    [[nodiscard]] int release() {
        int fd_ = fd;
        fd = -1;
        return fd_;
    }

private:
    int fd;

};

} // namespace mboxid

#endif // LIBMBOXID_UNIQUE_FD_HPP
