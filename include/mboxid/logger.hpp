// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_LOGGER_HPP
#define LIBMBOXID_LOGGER_HPP

#include <memory>
#include <string_view>

namespace mboxid {

class logger_base {
public:
    virtual void debug(std::string_view msg) const = 0;
    virtual void info(std::string_view msg) const = 0;
    virtual void warning(std::string_view msg) const = 0;
    virtual void error(std::string_view msg) const = 0;
    virtual void auth(std::string_view msg) const = 0;

    virtual ~logger_base() = default;
};

using logger_unique_ptr = std::unique_ptr<const logger_base>;

logger_unique_ptr make_standard_logger();
void install_logger(logger_unique_ptr new_logger);

} // namespace mboxid

#endif // LIBMBOXID_LOGGER_HPP
