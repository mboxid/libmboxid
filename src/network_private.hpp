// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_NETWORK_PRIVATE_HPP
#define LIBMBOXID_NETWORK_PRIVATE_HPP

#include <memory>
#include <list>
#include <mboxid/network.hpp>

namespace mboxid::net {

struct endpoint {
    std::unique_ptr<struct sockaddr> addr;
    socklen_t addrlen;
    int family;
    int socktype;
    int protocol;
};

enum class endpoint_usage { passive_open, active_open };

std::list<endpoint> resolve_endpoint(const char* host, const char* service,
        ip_protocol_version ip_version, endpoint_usage usage);

} // namespace mboxid::net

#endif // LIBMBOXID_NETWORK_PRIVATE_HPP
