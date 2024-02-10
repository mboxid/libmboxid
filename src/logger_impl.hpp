// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_LOGGER_IMPL_HPP
#define LIBMBOXID_LOGGER_IMPL_HPP

#include <memory>
#include <fmt/core.h>
#include <mboxid/logger.hpp>

namespace mboxid {

extern logger_unique_ptr logger;

template<typename... Args>
void log_debug(std::string_view fmt, Args&&... args) {
    logger->debug(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template<typename... Args>
void log_info(std::string_view fmt, Args&&... args) {
    logger->info(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template<typename... Args>
void log_warning(std::string_view fmt, Args&&... args) {
    logger->warning(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template<typename... Args>
void log_error(std::string_view fmt, Args&&... args) {
    logger->error(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template<typename... Args>
void log_auth(std::string_view fmt, Args&&... args) {
    logger->auth(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

} // namespace mboxid

#endif // LIBMBOXID_LOGGER_IMPL_HPP
