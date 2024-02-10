// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include "error_impl.hpp"

namespace mboxid {

std::string modbus_cat::message(int ec) const {
    using errc = modbus_errc;
    switch (static_cast<errc>(ec)) {
    case errc::success: return "success";
    case errc::illegal_function: return "illegal function";
    default: return "unkown";
    }
}

const std::error_category& modbus_category() noexcept {
    static modbus_cat obj;
    return obj;
}

std::error_code make_error_code(modbus_errc e) noexcept {
    return std::error_code(static_cast<int>(e), modbus_category());
}

std::string general_cat::message(int ec) const {
    using errc = general_errc;
    switch (static_cast<errc>(ec)) {
    case errc::success: return "success";
    case errc::work_in_progress: return "work is in progress";
    case errc::invalid_argument: return "invalid argument";
    default: return "unkown";
    }
}

const std::error_category& general_category() noexcept {
    static general_cat obj;
    return obj;
}

std::error_code make_error_code(general_errc e) noexcept {
    return std::error_code(static_cast<int>(e), general_category());
}

} // namespace mboxid
