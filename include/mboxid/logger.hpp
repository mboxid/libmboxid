// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Interface to the library's logging facilities.
 */
#ifndef LIBMBOXID_LOGGER_HPP
#define LIBMBOXID_LOGGER_HPP

#include <memory>
#include <string_view>

namespace mboxid::log {

/*!
 * Logger base class.
 *
 * The library uses implementation of this class for its internal logging. The
 * standard logger provided by the library logs messages to stdout and stderr
 * respectively.
 *
 * Someone can provide their own logging sink by installing a customized logger
 * instance.
 */
class logger_base {
public:
    //! Log a debug message.
    virtual void debug(std::string_view msg) const = 0;
    //! Log an info message.
    virtual void info(std::string_view msg) const = 0;
    //! Log a warning.
    virtual void warning(std::string_view msg) const = 0;
    //! Log an error.
    virtual void error(std::string_view msg) const = 0;
    //! Log a security-related message.
    virtual void auth(std::string_view msg) const = 0;

    virtual ~logger_base() = default;
};

using logger_unique_ptr = std::unique_ptr<const logger_base>;

//! Create an instance of the standard logger.
logger_unique_ptr make_standard_logger();

//! Install a logger instance.
void install_logger(logger_unique_ptr new_logger);

/*!
 * Get access to the installed logger instance without taking ownership.
 * \internal
 * This function is provided for unit tests, but can also be used for other
 * purposes.
 */
const logger_base* borrow_logger();

} // namespace mboxid::log

#endif // LIBMBOXID_LOGGER_HPP
