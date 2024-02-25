// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_NETWORK_HPP
#define LIBMBOXID_NETWORK_HPP

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace mboxid::net {

enum class ip_protocol_version {
    any = AF_UNSPEC,
    v4 = AF_INET,
    v6 = AF_INET6,
};

//! Socket address of a Modbus TCP node in human readable format.
struct endpoint_addr {
    //! Name or IP address of the host running the service.
    std::string host;

    //! Port number or name of the service. If empty, the default port is used.
    std::string service;

    //! IP protocol version to use.
    ip_protocol_version ip_version = ip_protocol_version::any;
};

endpoint_addr to_endpoint_addr(const struct sockaddr* addr, socklen_t addrlen,
                               bool numeric = true);

} // mboxid

#endif // LIBMBOXID_NETWORK_HPP
