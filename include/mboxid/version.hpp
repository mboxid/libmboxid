// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_VERSION_HPP
#define LIBMBOXID_VERSION_HPP

namespace mboxid {

extern const char* get_version();
extern void get_version(int& major, int& minor, int& patch);
extern const char* get_verbose_version();

extern const char* get_vendor();
extern const char* get_product_name();

} // namespace mboxid

#endif // LIBMBOXID_VERSION_HPP
