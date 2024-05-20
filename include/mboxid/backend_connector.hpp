// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 *\file
 * Connector between Modbus server and user application.
 *
 * This file provides a class which connects the Modbus server with the user
 * application.
 */
#ifndef LIBMBOXID_BACKEND_CONNECTOR_HPP
#define LIBMBOXID_BACKEND_CONNECTOR_HPP

#include <iostream>
#include <cstdint>
#include <vector>
#include <mboxid/network.hpp>
#include <mboxid/error.hpp>
#include <mboxid/version.hpp>

namespace mboxid {

/*!
 * Modbus backend interface.
 *
 * This base class defines the backend interface for the Modbus server. It
 * connects the server with the user application. To use it derive from the
 * class and implement its virtual functions according to your needs.
 */
class backend_connector {
public:
    //! Type used as client identifier.
    typedef std::uint64_t client_id;

    //! Default constructor.
    backend_connector() = default;

    /*!
     * Disable copy constructor.
     *
     * In favor of clear ownership, we prevent copies of instances of this
     * class. We suggest to move them instead.
     */
    backend_connector(const backend_connector&) = delete;

    /*!
     * Disable copy-assignment operator.
     *
     * In favor of clear ownership, we prevent copies of instances of this
     * class. We suggest to move them instead.
     */
    backend_connector& operator=(const backend_connector&) = delete;

    //! Move constructor.
    backend_connector(backend_connector&&) = default;

    //! Move assignment operator.
    backend_connector& operator=(backend_connector&&) = default;

    //! Default destructor.
    virtual ~backend_connector() = default;

    /*!
     * Authorize a Modbus client.
     *
     * This method is invoked by the server every time a client connects to
     * the server. It asks the user application either to accept or reject
     * the client.
     *
     * \param[in] id Unique identifier of the client.
     * \param[in] numeric_client_addr
     *      The address of the client in a human readable numeric format.
     * \param[in] addr
     *      Address of the client as socket structure address as from the
     *      socket API.
     * \param[in] addrlen
     *      The actual size of \a addr.
     * \return True to accept the client, otherwise false to reject the client.
     */
    virtual bool authorize([[maybe_unused]] client_id id,
            [[maybe_unused]] const net::endpoint_addr& numeric_client_addr,
            [[maybe_unused]] const sockaddr* addr,
            [[maybe_unused]] socklen_t addrlen) {
        return true;
    }

    /*!
     * Indicates that a client connection has been closed.
     *
     * \param[in] id Unique identifier of the client.
     */
    virtual void disconnect([[maybe_unused]] client_id id) {}

    /*!
     * Indicates that the client has successfully polled the server.
     *
     * \param[in] id Unique identifier of the client.
     */
    virtual void alive([[maybe_unused]] client_id id) {}

    /*!
     * Backend ticker invoked approximately once a second.
     *
     * This method may be overridden to implement individual inactivity
     * timeouts for clients, or to provide some kind of health monitoring.
     */
    virtual void ticker() {}

    /*!
     * \defgroup backend_connector_usual_return_values
     *
     * \return
     *      errc::none on success, otherwise a code describing the error. If
     *      the error code is a Modbus exception it is serialized and sent to
     *      the client, otherwise the error is logged by the server and the
     *      connection is closed.
     * \retval errc::none
     *      Success
     * \retval errc:modbus_exception_illegal_function
     *      Function not implemented by the server application.
     * \retval errc::modbus_exception_illegal_data_address
     *      The data address is not an allowable address.
     * \retval errc::modbus_exception_server_device_failure
     *      An unrecoverable error occurred while the server application
     *      was attempting to perform the requested action.
     * \retval errc::*
     *      Some other error.
     */

    /*!
     * Read status of contiguous coils.
     *
     * This method is invoked by the server when a client requests the
     * status of a single coil or a number of contiguous coils.
     *
     * \param[in] addr Address of the first coil.
     * \param[in] cnt Number of contiguous coils to read.
     * \param[out] bits Status of the requested coils
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc read_coils([[maybe_unused]] unsigned addr,
            [[maybe_unused]] std::size_t cnt,
            [[maybe_unused]] std::vector<bool>& bits) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Read status of contiguous discrete inputs.
     *
     * This method is invoked by the server when a client requests the
     * status of a discrete input or a number of contiguous discrete inputs.
     *
     * \param[in] addr Address of the first discrete input.
     * \param[in] cnt Number of contiguous discrete inputs to read.
     * \param[out] bits Status of the requested discrete inputs.
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc read_discrete_inputs([[maybe_unused]] unsigned addr,
            [[maybe_unused]] std::size_t cnt,
            [[maybe_unused]] std::vector<bool>& bits) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Read status of contiguous holding registers.
     *
     * This method is invoked by the server when a client requests the
     * status of a single holding register or a number of contiguous holding
     * registers.
     *
     * \param[in] addr Address of the first holding register.
     * \param[in] cnt Number of contiguous holding registers to read.
     * \param[out] regs Status of the requested holding registers.
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc read_holding_registers([[maybe_unused]] unsigned addr,
            [[maybe_unused]] std::size_t cnt,
            [[maybe_unused]] std::vector<std::uint16_t>& regs) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Read status of contiguous input registers.
     *
     * This method is invoked by the server when a client requests the
     * status of a single input register or a number of contiguous input
     * registers.
     *
     * \param[in] addr Address of the first input register.
     * \param[in] cnt Number of contiguous input registers to read.
     * \param[out] regs Status of the requested input registers.
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc read_input_registers([[maybe_unused]] unsigned addr,
            [[maybe_unused]] std::size_t cnt,
            [[maybe_unused]] std::vector<std::uint16_t>& regs) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Write to coils.
     *
     * This method is invoked by the server when a client requests to
     * write to a single coil or a number of contiguous coils.
     *
     * \param[in] addr Address of the first coil.
     * \param[in] bits Values to write to the coils.
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc write_coils([[maybe_unused]] unsigned addr,
            [[maybe_unused]] const std::vector<bool>& bits) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Write to holding registers.
     *
     * This method is invoked by the server when a client requests to
     * write to a holding register or a number of contiguous holding registers.
     *
     * \param[in] addr Address of the first holding register.
     * \param[in] regs Values to write to the holding registers.
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc write_holding_registers([[maybe_unused]] unsigned addr,
            [[maybe_unused]] const std::vector<std::uint16_t>& regs) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Write to and read from holding registers.
     *
     * This method is invoked by the server when a client requests to
     * write to, and subsequently read from a holding register or a number of
     * contiguous holding registers. Being said, this method is invoked for
     * the Modbus function "(0x17) Read/Write Multiple registers".
     *
     * For data consistency reasons, the server does not call
     * read_holding_register() and write_holding_register()
     * independently of each other, but demands to implement this method.
     *
     * To comply with the Modbus specification the write operation must
     * be executed before the read operation.
     *
     * \param[in] addr_wr Address of the first holding register to write to.
     * \param[in] regs_wr Values to write to the holding registers.
     * \param[in] addr_rd Address of the first holding register to read from.
     * \param[in] cnt_rd Number of contiguous holding registers to read.
     * \param[out] regs_rd Status of the requested holding registers.
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc write_read_holding_registers([[maybe_unused]] unsigned addr_wr,
            [[maybe_unused]] const std::vector<std::uint16_t>& regs_wr,
            [[maybe_unused]] unsigned addr_rd,
            [[maybe_unused]] std::size_t cnt_rd,
            [[maybe_unused]] std::vector<std::uint16_t>& regs_rd) {
        return errc::modbus_exception_illegal_function;
    }

    /*!
     * Read basic device identification.
     *
     * This method is invoked by the sever when the client requests to read
     * the device identification.
     *
     * \note
     * The server currently only supports basic device identification. The
     * returned vendor, product and version information must be short enough to
     * fit into a single Modbus frame.
     *
     *
     * \param[out] vendor Vendor of the server application.
     * \param[out] product Product name.
     * \param[out] version Version of the server application.
     * \return
     *
     * \copydetails backend_connector_usual_return_values
     */
    virtual errc get_basic_device_identification(
            std::string& vendor, std::string& product, std::string& version) {
        vendor = get_vendor();
        product = get_product_name();
        version = get_version();
        return errc::none;
    }
};

} // namespace mboxid

#endif // LIBMBOXID_BACKEND_CONNECTOR_HPP
