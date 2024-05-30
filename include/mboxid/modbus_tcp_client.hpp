// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Modbus TCP/IP client API.
 */
#ifndef LIBMBOXID_MODBUS_TCP_CLIENT_HPP
#define LIBMBOXID_MODBUS_TCP_CLIENT_HPP

#include <memory>
#include <string>
#include <vector>
#include <mboxid/common.hpp>
#include <mboxid/error.hpp>
#include <mboxid/network.hpp>

namespace mboxid {

/*!
 * Modbus TCP/IP client class.
 */
class modbus_tcp_client {
public:
    struct context;

    //! Default constructor.
    modbus_tcp_client();

    /*!
     * Disable copy constructor.
     *
     * In favor of clear ownership, we prevent copies of instances of this
     * class. We suggest to move them instead.
     */
    modbus_tcp_client(const modbus_tcp_client&) = delete;

    /*!
     * Disable copy-assignment operator.
     *
     * In favor of clear ownership, we prevent copies of instances of this
     * class. We suggest to move them instead.
     */
    modbus_tcp_client& operator=(const modbus_tcp_client&) = delete;

    //! Move constructor.
    modbus_tcp_client(modbus_tcp_client&&) = default;

    //! Move assignment operator.
    modbus_tcp_client& operator=(modbus_tcp_client&&) = default;

    //! Default destructor.
    ~modbus_tcp_client();

    /*!
     * Connect to the Modbus server.
     *
     * This method calls getaddrinfo() to look up the socket address(es) of
     * \a service running on \a host and tries to connect to it. getaddrinfo()
     * may return multiple addresses, e.g. one for IPv4 and another for IPv6.
     * The addresses are tried one after the other to establish a connection.
     * If none of them works, an exception is thrown.
     *
     * \param[in] host Name or IP address of the server.
     * \param[in] service Port number or name of the service.
     * \param[in] ip_version The version of the IP protocol to use.
     * \param[in] timeout
     *      User-specific time limit in [millisecond] for establishing a
     *      connection. The timeout is per socket address tried to connect to.
     *      If \c no_timeout is passed the timeout built into the systems
     *      TCP/IP stack takes effect. The latter is recommended unless there
     *      are good reasons not to do so. If necessary, select a timeout
     *      around 1 minute or more, as otherwise the mechanisms in TCP that
     *      ensure reliable communication will be ineffective.
     *
     * \throw mboxid::system_error
     *      A system call returned an error that is not handled further by the
     *      library.
     * \throw mboxid::mboxid_error(errc::active_open_error)
     *      Failed to connect to the Modbus server.
     * \throw mboxid::mboxid_error(errc::*)
     *      Some other error.
     */
    void connect_to_server(const std::string& host,
            const std::string& service = "",
            net::ip_protocol_version ip_version = net::ip_protocol_version::any,
            milliseconds timeout = no_timeout);

    //! Disconnect from Modbus server.
    void disconnect();

    /*!
     * Sets the time limit for responses.
     *
     * This method allows to set a time limit within which the expected
     * response from the server must be received in full. Select a timeout of
     * at least several seconds, as otherwise the mechanisms in TCP that ensure
     * reliable communication will be ineffective. The timeout itself is
     * specified in [millisecond] as this is the resolution used by the
     * underlying system calls.
     *
     * \param[in] timeout Time limit for the response in [millisecond].
     */
    void set_response_timeout(milliseconds timeout);

    //! Sets the Modbus unit identifier of this client.
    void set_unit_id(unsigned id);

    /*!
     * \defgroup client_exceptions_doc Modbus client class common exceptions
     *
     * \throw mboxid::system_error
     *      A system call returned an error that is not handled further by the
     *      library.
     * \throw mboxid::mboxid_error(errc::not_connected)
     *      The client is not connected to a Modbus server.
     * \throw mboxid::mboxid_error(errc::connection_closed)
     *      The connection to the Modbus server has been closed.
     * \throw mboxid::mboxid_error(errc::timeout)
     *      Timout for server response occurred.
     * \throw mboxid::mboxid_error(errc::parse_error)
     *      The response from the server is malformed.
     * \throw mboxid::mboxid_error(errc::*)
     *      Some other error.
     */

    /*!
     * Queries the state of coils from the server.
     *
     * \param[in] addr
     *      Address of the first coil (counting starts with 0).
     * \param[in] cnt
     *      Number of coils to read.
     * \return State of the coils.
     *
     * \copydetails client_exceptions_doc
     */
    std::vector<bool> read_coils(unsigned addr, size_t cnt);

    /*!
     * Queries the state of discrete inputs from the server.
     *
     * \param[in] addr
     *      Address of the first discrete input (counting starts with 0).
     * \param[in] cnt
     *      Number of discrete inputs to read.
     * \return State of the discrete inputs.
     *
     * \copydetails client_exceptions_doc
     */
    std::vector<bool> read_discrete_inputs(unsigned addr, size_t cnt);

    /*!
     * Queries the state of holding registers from the server.
     *
     * \param[in] addr
     *      Address of the first holding register (counting starts with 0).
     * \param[in] cnt
     *      Number of holding registers to read.
     * \return State of the holding registers.
     *
     * \copydetails client_exceptions_doc
     */
    std::vector<uint16_t> read_holding_registers(unsigned addr, size_t cnt);

    /*!
     * Queries the state of input registers from the server.
     *
     * \param[in] addr
     *      Address of the first input register (counting starts with 0).
     * \param[in] cnt
     *      Number of input registers to read.
     * \return State of the input registers.
     *
     * \copydetails client_exceptions_doc
     */
    std::vector<uint16_t> read_input_registers(unsigned addr, size_t cnt);

    /*!
     * Issues a write operation to a single coil.
     *
     * \param[in] addr Address of the coil (counting starts with 0).
     * \param[in] on If true 1 is written to the coil, otherwise 0.
     *
     * \copydetails client_exceptions_doc
     */
    void write_single_coil(unsigned addr, bool on);

    /*!
     * Issues a write operation to a single holding register.
     *
     * \param[in] addr Address of the register (counting starts with 0).
     * \param[in] val The value to write to the register.
     *
     * \copydetails client_exceptions_doc
     */
    void write_single_register(unsigned addr, unsigned val);

    /*!
     * Issues write operations to a number of contiguous coils.
     *
     * \param[in] addr Address of the first coil (counting starts with 0).
     * \param[in] bits List of values to write to the coils.
     *
     * \copydetails client_exceptions_doc
     */
    void write_multiple_coils(unsigned addr, const std::vector<bool>& bits);

    /*!
     * Issues write operations to a number of contiguous holding registers.
     *
     * \param[in] addr
     *      Address of the first holding register (counting starts with 0).
     * \param[in] regs
     *      List of values to write to the holding registers.
     *
     * \copydetails client_exceptions_doc
     */
    void write_multiple_registers(
            unsigned addr, const std::vector<uint16_t>& regs);

    /*!
     * Modifies a holding register by applying bitwise operations.
     *
     * \param[in] addr Address of the holding register (counting starts with 0).
     * \param[in] and_msk Mask for a bitwise _AND_ operation.
     * \param or_msk Mask for a bitwise _OR_ operation.
     *
     * \copydetails client_exceptions_doc
     *
     * The register is modified as follows:
     * \code
     *      reg = (reg _AND_ and_msk) _OR_ (or_msk _AND_ (_NOT_ and_msk))
     * \endcode
     */
    void mask_write_register(unsigned addr, unsigned and_msk, unsigned or_msk);

    /*!
     * Writes to holding registers followed by reading from holding registers.
     *
     * \param[in] addr_wr
     *      Address of the first holding register to write to (counting starts
     *      with 0).
     * \param[in] regs_wr
     *      List of values to write to the holding registers.
     * \param[in] addr_rd
     *      Address of the first holding register to read from (counting starts
     *      with 0).
     * \param[in] cnt_rd
     *      Number of input registers to read.
     * \return State of the read holding registers.
     *
     * \copydetails client_exceptions_doc
     */
    std::vector<uint16_t> read_write_multiple_registers(unsigned addr_wr,
            const std::vector<uint16_t>& regs_wr, unsigned addr_rd,
            size_t cnt_rd);

    /*!
     * Reads the basic device identification of the server.
     *
     * \param[out] vendor Vendor of the server application.
     * \param[out] product Product name.
     * \param[out] version Version of the server application.
     *
     * \copydetails client_exceptions_doc
     */
    void read_device_identification(
            std::string& vendor, std::string& product, std::string& version);

private:
    std::unique_ptr<context> ctx;
};

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_TCP_CLIENT_HPP
