// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_ERROR_PRIVATE_HPP
#define LIBMBOXID_ERROR_PRIVATE_HPP

#include <mboxid/error.hpp>

namespace mboxid {

static inline void validate_argument(bool cond, const char* msg) {
    if (!cond)
        throw (mboxid_error(errc::invalid_argument, msg));
}

static inline void expects(bool cond, const char* msg) {
    if (!cond)
        throw (mboxid_error(errc::logic_error, msg));
}

} // namespace mboxid

#endif // LIBMBOXID_ERROR_PRIVATE_HPP
