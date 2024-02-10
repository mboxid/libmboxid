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
class Unique_fd {
public:
    Unique_fd(const Unique_fd&) = delete;
    Unique_fd& operator=(const Unique_fd&) = delete;

    Unique_fd() : fd{-1} {}
    explicit Unique_fd(int fd) : fd{fd} {}

    Unique_fd(Unique_fd&& other) : fd{other.release()} {}

    Unique_fd& operator=(Unique_fd&& other) {
        reset(other.release());
        return *this;
    }

    ~Unique_fd() {
        reset();
    }

    int get() const { return fd; }

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
