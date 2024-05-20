// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
/*!
 * \file
 * Some useful types and functions for networking purposes.
 */
#ifndef LIBMBOXID_NETWORK_HPP
#define LIBMBOXID_NETWORK_HPP

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace mboxid::net {

//! Enumeration to specify the Internet Protocol (IP) version.
enum class ip_protocol_version {
    any = AF_UNSPEC,    //!< Version unspecified, allow any of them.
    v4 = AF_INET,       //!< IP version 4.
    v6 = AF_INET6,      //!< IP version 6.
};

//! Address of a Modbus TCP node in human readable format.
struct endpoint_addr {
    std::string host;
        //!< Name or IP address of the host running the service.
    std::string service;
        //!< Port number or name of the service.
    ip_protocol_version ip_version = ip_protocol_version::any;
        //!< IP protocol version to use.
};

/*!
 * Converts socket structure address into a human readable format.
 *
 * This functions converts a socket structure address from the socket API into
 * a human readable format.
 *
 * \param[in] addr socket structure address from the socket API.
 * \param[in] addrlen The actual size of \a addr.
 * \param[in] numeric
 *      If true the numeric form of host and service are returned as string,
 *      otherwise the function will try to lookup the name of the host and
 *      the service.
 * \return Socket address in human readable format.
 */
endpoint_addr to_endpoint_addr(
        const struct sockaddr* addr, socklen_t addrlen, bool numeric = true);

} // namespace mboxid::net

#endif // LIBMBOXID_NETWORK_HPP
