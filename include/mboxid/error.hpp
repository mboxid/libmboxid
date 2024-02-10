// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_ERROR_HPP
#define LIBMBOXID_ERROR_HPP

#include <stdexcept>
#include <system_error>

namespace mboxid {

enum class modbus_errc {
    success = 0,
    illegal_function,
};

class modbus_cat : public std::error_category {
public:
    const char* name() const noexcept override { return "modbus"; }
    std::string message(int ec) const override;
};

const std::error_category& modbus_category() noexcept;

std::error_code make_error_code(modbus_errc e) noexcept;

enum class general_errc {
    success = 0,
    work_in_progress,
    invalid_argument,
};

class general_cat : public std::error_category {
public:
    const char* name() const noexcept override { return "general"; }
    std::string message(int ec) const override;
};

const std::error_category& general_category() noexcept;

std::error_code make_error_code(general_errc e) noexcept;

/// Exception signaling an error that originate from the operating system.
class system_error : public std::system_error {
public:
    system_error(int ev, const char* what_arg)
        : std::system_error(ev, std::system_category(), what_arg) {}
};

class modbus_error : public std::system_error {
public:
    modbus_error(modbus_errc errc, const char* what_arg)
        : std::system_error(make_error_code(errc), what_arg) {}

};

class general_error : public std::system_error {
public:
    general_error(general_errc errc, const char* what_arg)
        : std::system_error(make_error_code(errc), what_arg) {}

};

} // namespace mboxid

template<>
struct std::is_error_code_enum<mboxid::modbus_errc> : public true_type {};

template<>
struct std::is_error_code_enum<mboxid::general_errc> : public true_type {};

#endif // LIBMBOXID_ERROR_HPP
