// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>
#include "error_private.hpp"
#include "logger_private.hpp"

namespace mboxid::log {

constexpr auto prefix = "libmboxid: ";

class standard_logger : public logger_base {
public:
    void debug(std::string_view msg) const final {
        std::cout << prefix << "debug: " << msg << "\n";
    }

    void info(std::string_view msg) const final {
        std::cout << prefix << "info: " << msg << "\n";
    }

    void warning(std::string_view msg) const final {
        std::cout << prefix << "warning: " << msg << "\n";
    }

    void error(std::string_view msg) const final {
        std::cerr << prefix << "error: " << msg << "\n";
    }

    void auth(std::string_view msg) const final {
        std::cout << prefix << "auth: " << msg << "\n";
    }

    ~standard_logger() final = default;
};

logger_unique_ptr logger = std::make_unique<standard_logger>();

logger_unique_ptr make_standard_logger() {
    return std::make_unique<standard_logger>();
}

void install_logger(logger_unique_ptr new_logger) {
    validate_argument(new_logger.get(), "install_logger()");
    logger = std::move(new_logger);
}

} // namespace mboxid
