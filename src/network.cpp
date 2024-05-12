// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <cstring>
#include "error_private.hpp"
#include "network_private.hpp"
#include "logger_private.hpp"

namespace mboxid::net {

static bool sort_by_addr(const endpoint& ep1, const endpoint& ep2) {
    if (ep1.addrlen < ep2.addrlen)
        return true;
    else if (ep1.addrlen == ep2.addrlen)
        return (std::memcmp(ep1.addr.get(), ep2.addr.get(), ep1.addrlen) < 0);
    else
        return false;
}

static bool compare_by_addr(const endpoint& ep1, const endpoint& ep2) {
    if (ep1.addrlen != ep2.addrlen)
        return false;
    return (std::memcmp(ep1.addr.get(), ep2.addr.get(), ep1.addrlen) == 0);
}

std::list<endpoint> resolve_endpoint(
    const char* host, const char* service, ip_protocol_version ip_version,
    endpoint_usage usage)
{
    using namespace std::string_literals;

    struct addrinfo hints{};

    hints.ai_flags = AI_ADDRCONFIG;
    if (usage == endpoint_usage::passive_open)
        hints.ai_flags |= AI_PASSIVE;
    hints.ai_family = static_cast<int>(ip_version);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *result;
    int err;
    do {
        err = getaddrinfo(host, service, &hints, &result);
    } while ((err == EAI_SYSTEM) && errno == EINTR);

    if (err == EAI_SYSTEM)
        throw system_error(errno, "getaddrinfo");
    else if (err)
        throw mboxid_error(errc::gai_error,
                            "getaddrinfo: "s + gai_strerror(err));

    std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)>
        ai_free_guard(result, freeaddrinfo);

    std::list<endpoint> endpoints;
    for (auto rp = result; rp; rp = rp->ai_next) {
        endpoint ep;

        auto sa = new struct sockaddr_storage();
        std::memcpy(sa, rp->ai_addr, rp->ai_addrlen);

        ep.addr.reset(reinterpret_cast<struct sockaddr*>(sa));
        ep.addrlen = rp->ai_addrlen;
        ep.family = rp->ai_family;
        ep.socktype = rp->ai_socktype;
        ep.protocol = rp->ai_protocol;

        endpoints.push_back(std::move(ep));
    }

    // unfortunately getaddrinfo() may return duplicate results
    // see: https://www.openldap.org/lists/openldap-bugs/200711/msg00169.html
    endpoints.sort(sort_by_addr);
    endpoints.unique(compare_by_addr);
    return endpoints;
}

endpoint_addr to_endpoint_addr(const struct sockaddr* addr, socklen_t addrlen,
                             bool numeric)
{
    using namespace std::string_literals;

    validate_argument(
        (addr->sa_family == AF_INET) || (addr->sa_family == AF_INET6),
        "to_endpoint_addr");

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    int err;

    do {
        err = getnameinfo(addr, addrlen, host, sizeof(host),
                    serv, sizeof(serv), numeric ? NI_NUMERICHOST : 0);
    } while ((err == EAI_SYSTEM) && (errno == EINTR));

    if (err == EAI_SYSTEM)
        throw system_error(errno, "getnameinfo");
    else if (err)
        throw mboxid_error(errc::gai_error,
                            "getnameinfo: "s + gai_strerror(err));

    endpoint_addr res;
    res.host = host;
    res.service = serv;
    res.ip_version = static_cast<ip_protocol_version>(addr->sa_family);

    return res;
}

} // namespace mboxid
