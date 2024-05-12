// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_COMMON_HPP
#define LIBMBOXID_COMMON_HPP

#include <cstddef>
#include <cstdint>
#include <chrono>

namespace mboxid {

// The following types are frequently used in the project. Unfortunately, it
// compiler-specific whether they are in the global namespace or not.
// Therefore, we bring them into our own namespace.
using std::uint8_t;
using std::uint16_t;
using std::size_t;

using std::chrono::milliseconds;

constexpr auto no_timeout = milliseconds::max();

constexpr const char* server_default_port = "502";
constexpr const char* secure_server_default_port = "802";

} // namespace mboxid

#endif // LIBMBOXID_COMMON_HPP
