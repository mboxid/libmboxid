// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_ERROR_HPP
#define LIBMBOXID_ERROR_HPP

#include <stdexcept>
#include <system_error>

namespace mboxid {

enum class errc {
    none = 0,

    // mobus protocol exception
    modbus_exception_illegal_function,
    modbus_exception_illegal_data_address,
    modbus_exception_illegal_data_value,
    modbus_exception_slave_or_server_failure,
    modbus_exception_acknowledge,
    modbus_exception_slave_or_server_busy,
    modbus_exception_negative_acknowledge,
    modbus_exception_memory_parity,
    modbus_exception_not_defined,
    modbus_exception_gateway_path,
    modbus_exception_gateway_target,

    // libmboxid native errors
    invalid_argument,
    logic_error,
    gai_error,
    passive_open_error,
    active_open_error,
    parse_error,
};

const std::error_category& mboxid_category() noexcept;

std::error_code make_error_code(errc e) noexcept;

/// Base class for libmoxid exceptions.
class exception : std::system_error {
public:
    using std::system_error::system_error;
    using std::system_error::code;
    using std::system_error::what;
};

/// Exception signaling an error that originate from the operating system.
class system_error : public exception {
public:
    system_error(int ev)
        : exception(ev, std::system_category()) {}

    system_error(int ev, const char* what_arg)
        : exception(ev, std::system_category(), what_arg) {}

    system_error(int ev, const std::string& what_arg)
        : exception(ev, std::system_category(), what_arg) {}
};

class mboxid_error : public exception {
public:
    mboxid_error(errc errc)
        : exception(make_error_code(errc)) {}

    mboxid_error(errc errc, const char* what_arg)
        : exception(make_error_code(errc), what_arg) {}

    mboxid_error(errc errc, const std::string& what_arg)
        : exception(make_error_code(errc), what_arg) {}

};

static inline bool is_modbus_exception(errc e) {
    return (e > errc::none) && (e < errc::invalid_argument);
}

static inline bool is_modbus_exception(const exception& e) {
    return (e.code().category() == mboxid_category()) &&
            is_modbus_exception(static_cast<errc>(e.code().value()));
}

} // namespace mboxid

namespace std {

template <>
struct is_error_code_enum<mboxid::errc> : public true_type {};

} // namespace std

#endif // LIBMBOXID_ERROR_HPP
