// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Common types and constants used throughout the library.
 */
#ifndef LIBMBOXID_COMMON_HPP
#define LIBMBOXID_COMMON_HPP

#include <cstddef>
#include <cstdint>
#include <chrono>

namespace mboxid {

/*!
 * Fixed width integer types.
 *
 * The following types are frequently used in the project. Unfortunately, it
 * is compiler-specific whether they are in the global namespace or not.
 * Therefore, we bring them into the mboxid namespace.
 * @{
 */
using std::uint8_t;
using std::uint16_t;
using std::size_t;
//! @}

//! Type used to specify timeouts and durations.
using std::chrono::milliseconds;

//! Constant to disable a timeout.
constexpr auto no_timeout = milliseconds::max();

//! Default Modbus TCP/IP server port.
constexpr const char* server_default_port = "502";

//! Default Modbus/TCP Security port.
constexpr const char* secure_server_default_port = "802";

} // namespace mboxid

#endif // LIBMBOXID_COMMON_HPP
