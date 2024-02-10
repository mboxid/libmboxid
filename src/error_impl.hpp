// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_ERROR_IMPL_HPP
#define LIBMBOXID_ERROR_IMPL_HPP

#include <mboxid/error.hpp>

namespace mboxid {

static inline void validate_argument(bool cond, const char* msg) {
    if (!cond)
        throw (general_error(general_errc::invalid_argument, msg));
}

} // namespace mboxid

#endif // LIBMBOXID_ERROR_IMPL_HPP
