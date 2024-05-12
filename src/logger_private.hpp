// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_LOGGER_PRIVATE_HPP
#define LIBMBOXID_LOGGER_PRIVATE_HPP

#include <memory>
#include <fmt/core.h>
#include <mboxid/logger.hpp>

namespace mboxid::log {

extern logger_unique_ptr logger;

template <typename... Args> void debug(std::string_view fmt, Args&&... args) {
    logger->debug(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args> void info(std::string_view fmt, Args&&... args) {
    logger->info(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args> void warning(std::string_view fmt, Args&&... args) {
    logger->warning(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args> void error(std::string_view fmt, Args&&... args) {
    logger->error(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... Args> void auth(std::string_view fmt, Args&&... args) {
    logger->auth(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

} // namespace mboxid::log

#endif // LIBMBOXID_LOGGER_PRIVATE_HPP
