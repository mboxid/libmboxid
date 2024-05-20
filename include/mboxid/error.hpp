// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Error codes and exceptions.
 */
#ifndef LIBMBOXID_ERROR_HPP
#define LIBMBOXID_ERROR_HPP

#include <stdexcept>
#include <system_error>

namespace mboxid {

//! Enumeration of errors.
enum class errc {
    none = 0,
        //!< Success. No error occurred.

    modbus_exception_illegal_function,
        //!< Modbus exception: The function code received in the query is not
        //!< an allowable action for the server.
    modbus_exception_illegal_data_address,
        //!< Modbus exception: The data address received in the query is not an
        //!< allowable address for the server.
    modbus_exception_illegal_data_value,
        //!< Modbus exception: A value contained in the query data field is not
        //!< an allowable value for the server.
    modbus_exception_server_device_failure,
        //!< Modbus exception: An unrecoverable error occurred while the server
        //!< was attempting to perform the requested action.
    modbus_exception_acknowledge,
        //!< Modbus exception: The server has accepted the request and is
        //!< processing it, but a long duration of time will be required to
        //!< do so.
    modbus_exception_server_device_busy,
        //!< Modbus exception: The server is currently busy. The client should
        //!< retransmit the message later when the server is free.
    modbus_exception_negative_acknowledge,
        //!< Modbus exception: The slave cannot perform the program function
        //!< received in the query (deprecated).
    modbus_exception_memory_parity,
        //!< Modbus exception: The server attempted to read record file, but
        //!< detected a parity error in the memory.
    modbus_exception_not_defined,
        //!< Modbus exception: This is a placeholder as the code is not defined
        //!< in the Modbus specification.
    modbus_exception_gateway_path,
        //!< Modbus exception: The gateway was unable to allocate an internal
        //!< communication path from the input port to the output port for
        //!< processing the request.
    modbus_exception_gateway_target,
        //!< Modbus exception: The gateway did not obtain a response from the
        //!< target device.

    invalid_argument,
        //!< Native error: Invalid value was passed to the function/method.
    logic_error,
        //!< Native error: A condition has been detected that indicates a bug.
    gai_error,
        //!< Native error: A function of the getaddrinfo() function family
        //!< returned with an error not handled within the library.
    passive_open_error,
        //!< Native error: Server failed to bind to the specified TCP/IP port.
    active_open_error,
        //!< Native error: Client failed to connect to the server.
    parse_error,
        //!< Native error: Failure during parsing the Modbus request/response
        //!< frame.
    timeout,
        //!< Native error: Timeout occurred.
    not_connected,
        //!< Native error: Client is not connected to a server.
    connection_closed,
        //!< Native error: Connection has been closed.
};

//! Obtains a reference to libmboxid's static error category object.
const std::error_category& mboxid_category() noexcept;

//! Creates error code value for errc enum \a e.
std::error_code make_error_code(errc e) noexcept;

/*!
 * Base class for libmboxid exceptions.
 *
 * This is the base class for all exceptions thrown by this library. It
 * utilizes std::system_error to convey an error code together with a
 * descriptive messages.
 */
class exception : std::system_error {
public:
    using std::system_error::system_error;
    using std::system_error::code;
    using std::system_error::what;
};

/*!
 * Exception signaling an error that originates from the operating system.
 *
 * This exception is thrown when system calls return an error that are not
 * further handled by the library.
 */
class system_error : public exception {
public:
    explicit system_error(int ev) : exception(ev, std::system_category()) {}

    system_error(int ev, const char* what_arg)
            : exception(ev, std::system_category(), what_arg) {}

    system_error(int ev, const std::string& what_arg)
            : exception(ev, std::system_category(), what_arg) {}
};

/*!
 * Exception for errors mapped to mboxid::errc.
 *
 * The majority of exceptions thrown by the library is of this type. It is
 * used for all errors that are mapped to the mboxid::errc enumeration.
 */
class mboxid_error : public exception {
public:
    explicit mboxid_error(errc errc) : exception(make_error_code(errc)) {}

    mboxid_error(errc errc, const char* what_arg)
            : exception(make_error_code(errc), what_arg) {}

    mboxid_error(errc errc, const std::string& what_arg)
            : exception(make_error_code(errc), what_arg) {}
};

//! Test if the errc enum \a e represents a Modbus protocol exception.
static inline bool is_modbus_exception(errc e) {
    return (e > errc::none) && (e < errc::invalid_argument);
}

/*!
 * Test if exception \a e represents a Modbus protocol exception.
 *
 * \internal
 * CLion complains:
 *      All calls of function 'is_modbus_exception' are unreachable
 *
 * Actually, this is a false positive. Unit tests proved that the call is
 * reached.
 *
 * Unfortunately, we did not find a way to turn that warning of without
 * generating tons of gcc warnings about unknown pragmas.
 */
static inline bool is_modbus_exception(const exception& e) {
    return ((e.code().category() == mboxid_category()) &&
            is_modbus_exception(static_cast<errc>(e.code().value())));
}

} // namespace mboxid

namespace std {

// Specialization of template is_error_code<T> to indicate that the type
// mboxid::errc is eligible for std::error_code.
template <> struct is_error_code_enum<mboxid::errc> : public true_type {};

} // namespace std

#endif // LIBMBOXID_ERROR_HPP