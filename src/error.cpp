// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include "error_private.hpp"

namespace mboxid {

class mboxid_cat : public std::error_category {
public:
    const char* name() const noexcept override { return "mboxid"; }
    std::string message(int ec) const override;
};

std::string mboxid_cat::message(int ec) const {
    switch (static_cast<errc>(ec)) {
    case errc::none:
        return "success";
    case errc::modbus_exception_illegal_function:
        return "modbus exception illegal function";
    case errc::modbus_exception_illegal_data_address:
        return "modbus exception illegal data address";
    case errc::modbus_exception_illegal_data_value:
        return "modbus exception illegal data value";
    case errc::modbus_exception_slave_or_server_failure:
        return "modbus exception slave or server failure";
    case errc::modbus_exception_acknowledge:
        return "modbus exception acknowledge";
    case errc::modbus_exception_slave_or_server_busy:
        return "modbus exception slave or server busy";
    case errc::modbus_exception_negative_acknowledge:
        return "modbus exception negative acknowledge";
    case errc::modbus_exception_memory_parity:
        return "modbus exception memory parity";
    case errc::modbus_exception_not_defined:
        return "modbus exception not defined";
    case errc::modbus_exception_gateway_path:
        return "modbus exception gateway path";
    case errc::modbus_exception_gateway_target:
        return "modbus exception gateway target";
    case errc::invalid_argument:
        return "invalid argument";
    case errc::logic_error:
        return "logic error";
    case errc::gai_error:
        return "address info error";
    case errc::passive_open_error:
        return "passive open error";
    case errc::active_open_error:
        return "active open error";
    case errc::parse_error:
        return "parse error";
    case errc::timeout:
        return "timeout";
    case errc::connection_closed:
        return "connection closed or reset by peer";
    default:
        return "unkown";
    }
}

const std::error_category& mboxid_category() noexcept {
    static mboxid_cat obj;
    return obj;
}

std::error_code make_error_code(errc e) noexcept {
    return {static_cast<int>(e), mboxid_category()};
}

} // namespace mboxid
