// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Provides general information about the library.
 */
#ifndef LIBMBOXID_VERSION_HPP
#define LIBMBOXID_VERSION_HPP

namespace mboxid {

//! Returns the version number as C string.
extern const char* get_version();

//! Returns the major, minor and patch field of the version number.
extern void get_version(int& major, int& minor, int& patch);

//! Returns the library name together its version as C string.
extern const char* get_verbose_version();

//! Returns the organization that publishes the library as C string.
extern const char* get_vendor();

//! Returns the library name as C string.
extern const char* get_product_name();

} // namespace mboxid

#endif // LIBMBOXID_VERSION_HPP
