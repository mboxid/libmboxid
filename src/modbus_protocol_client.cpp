// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
#include "byteorder.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_client.hpp"

namespace mboxid {

using std::uint8_t;
using std::size_t;

static const std::map<function_code, size_t> min_rsp_pdu_size_by_fc {
    {function_code::read_coils, 3},
    {function_code::read_discrete_inputs, 3},
    {function_code::read_holding_registers, 4},
    {function_code::read_input_registers, 4},
    {function_code::write_single_coil, 5},
    {function_code::write_single_register, 5},
    {function_code::write_multiple_coils, 5},
    {function_code::write_multiple_registers, 5},
    {function_code::mask_write_register, 7},
    {function_code::read_write_multiple_registers, 4},
    {function_code::read_device_identification, 16},
    {function_code::exception, 2},
};

} // namespace mboxid
