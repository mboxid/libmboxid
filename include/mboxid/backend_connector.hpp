// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_BACKEND_CONNECTOR_HPP
#define LIBMBOXID_BACKEND_CONNECTOR_HPP

#include <iostream>
#include <cstdint>
#include <chrono>
#include <mboxid/network.hpp>

namespace mboxid {

class backend_connector {
public:
    using milliseconds = std::chrono::milliseconds;
    typedef std::uint64_t client_id;

    backend_connector() = default;
    backend_connector(const backend_connector&) = delete;
    backend_connector& operator=(const backend_connector&) = delete;
    backend_connector(backend_connector&&) = default;
    backend_connector& operator=(backend_connector&&) = default;
    virtual ~backend_connector() = default;

    virtual bool authorize(client_id id,
                           const net::endpoint_addr& numeric_client_addr,
                           const sockaddr* addr, socklen_t addrlen)
    {
        return true;
    }

    virtual void disconnect(client_id id) {}

    /**
     * Backend ticker is invoked approximately once a second.
     *
     * This method may be overridden to implement individual inactivity
     * timeouts for the clients, or to provide some kind of health monitoring.
     */
    virtual void ticker() { }

};

} // namespace mboxid

#endif // LIBMBOXID_BACKEND_CONNECTOR_HPP
