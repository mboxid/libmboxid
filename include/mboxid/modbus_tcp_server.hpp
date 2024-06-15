// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Modbus TCP/IP server API.
 */
#ifndef LIBMBOXID_MODBUS_TCP_SERVER_HPP
#define LIBMBOXID_MODBUS_TCP_SERVER_HPP

#include <memory>
#include <string>
#include <mboxid/common.hpp>
#include <mboxid/error.hpp>
#include <mboxid/network.hpp>
#include <mboxid/backend_connector.hpp>

namespace mboxid {

//! Modbus TCP/IP server class.
class modbus_tcp_server {
public:
    //! Type used as client identifier.
    using client_id = backend_connector::client_id;

    //! Default constructor.
    modbus_tcp_server();

    /*!
     * Disable copy constructor.
     *
     * In favor of clear ownership, we prevent copies of instances of this
     * class. We suggest to move them instead.
     */
    modbus_tcp_server(const modbus_tcp_server&) = delete;

    /*!
     * Disable copy-assignment operator.
     *
     * In favor of clear ownership, we prevent copies of instances of this
     * class. We suggest to move them instead.
     */
    modbus_tcp_server& operator=(const modbus_tcp_server&) = delete;

    //! Move constructor.
    modbus_tcp_server(modbus_tcp_server&&) = default;

    //! Move assignment operator.
    modbus_tcp_server& operator=(modbus_tcp_server&&) = default;

    //! Default destructor.
    ~modbus_tcp_server();

    /*!
     * Sets hints on the the server's own address.
     *
     * This method sets hints on the server's own address. The hints determine
     * to which interface(s) and port the server binds to.
     *
     * When run() is invoked getaddrinfo() is called to translate \a host,
     * \a service and \a ip_version in socket address structures.
     * getaddrinfo() may return several of these structures, e.g. when the
     * server has an IPv4 and an IPv6 address, and performs a passive open on
     * each of them.
     *
     * \param[in] host
     *      Name or IP address of the server. If empty, the server binds
     *      to any interface.
     * \param[in] service
     *      Port number or name of the service. If empty, the server uses
     *      the default server port for Modbus.
     * \param[in] ip_version
     *      The version of the IP protocol to use.
     */
    void set_server_addr(const std::string& host,
            const std::string& service = "",
            net::ip_protocol_version ip_version =
                    net::ip_protocol_version::any);

    /*!
     * Sets the backend which connects the server with the user application.
     *
     * This method sets the backend which connects the server with the user
     * application. The server takes ownership of the backend.
     */
    void set_backend(std::unique_ptr<backend_connector> backend);

    /*!
     * Get access to the backend without taking ownership.
     * \internal
     * This function is provided for unit tests, but can also be used for other
     * purposes.
     */
    backend_connector* borrow_backend();

    /*!
     * Server run loop.
     *
     * This method performs a passive open according to the server's
     * address(es). It accepts incoming connections and processes requests till
     * shutdown() is  called.
     *
     * It is suggested to execute run() in its own thread. It is safe to call
     * shutdown() from a different thread.
     *
     * \throw mboxid::system_error
     *      A system call returned an error that is not handled further by the
     *      library.
     * \throw mboxid::mboxid_error(errc::gai_error)
     *      A function of the getaddrinfo() function family returned with an
     *      error not handled within the library.
     * \throw mboxid::mboxid_error(errc::passive_open_error)
     *      Open listening port(s) failed.
     * \throw mboxid::mboxid_error(errc::*)
     *      Some other error.
     */
    void run();

    /*!
     * Asks the server to shut down its operation (thread-safe).
     *
     * This method queues a command which instructs the run() method to
     * shut down its operation and return. It is safe to call this method from
     * a different thread context than run().
     */
    void shutdown();

    /*!
     * Asks the server to close a connection (thread-safe).
     *
     * This method queues a command which instructs the run() method to
     * close the connection to the specified client \a id. It is safe to call
     * this method from a different thread context than run().
     *
     * @param[in] id Client identifier.
     */
    void close_client_connection(client_id id);

    //! Sets idle timeout after which to close a connection.
    void set_idle_timeout(milliseconds to);

    //! Sets the time limit within a request must be complete.
    void set_request_complete_timeout(milliseconds to);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_TCP_SERVER_HPP