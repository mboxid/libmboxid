// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <poll.h>
#include <memory>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <mboxid/modbus_tcp_client.hpp>
#include "unique_fd.hpp"
#include "network_private.hpp"
#include "logger_private.hpp"

namespace mboxid {

struct modbus_tcp_client::impl {
    unique_fd fd;
    bool use_tls = false;
};

modbus_tcp_client::modbus_tcp_client() : pimpl(std::make_unique<impl>()) {}

modbus_tcp_client::~modbus_tcp_client() = default;

// pre-condition: fd must be nonblocking

 /**
  * Try to connect to the sever within specified duration.
  *
  * @param fd Socket file descriptor (nonblocking).
  * @param addr Address of the server.
  * @param addrlen Size of the \a addr argument.
  * @param timeout no_timeout to disable the timeout, otherwise the maximum
  *     allowed duration to block.
  *
  * @return 0 on success, otherwise a POSIX error number (@see man 3 errno).
  *
  * \pre The socket file descriptor \a fd must be set to nonblocking.
  */
static int try_connect(int fd, const struct sockaddr* addr,
                                socklen_t addrlen, milliseconds timeout)
{
    int res;

    res = TEMP_FAILURE_RETRY(connect(fd, addr, addrlen));
    if (res == 0)
        throw mboxid_error(errc::logic_error, "connect: suspicious completion");

    // We expect connect() to return EINPROGRESS as the socket is nonblocking
    // and the connection cannot be completed immediately.
    if (errno != EINPROGRESS)
        return errno;

    // We poll() for completion by selecting the socket for writing,
    // applying a timeout if necessary.
    struct pollfd pollfd = {.fd = fd, .events = POLLOUT, .revents = 0};
    int to = (timeout == no_timeout) ? -1 : static_cast<int>(timeout.count());

    res = TEMP_FAILURE_RETRY(poll(&pollfd, 1, to));
    if (res < 0)
        throw system_error(errno, "poll");
    else if (res == 0)
        return ETIMEDOUT;
    else if ((res != 1) || !(pollfd.revents & POLLOUT))
        throw mboxid_error(errc::logic_error, "connect: socket not ready");

    // After poll() indicates writability, getsockopt() is used to read
    // SO_ERROR at level SOL_SOCKET to determine whether connect() completed
    // successfully or unsuccessfully.
    int optval;
    socklen_t optlen = sizeof(optval);
    res = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (res == -1)
        throw system_error(errno, "getsockopt");
    if (optlen != sizeof(optval))
        throw mboxid_error(errc::logic_error, "getsockopt: optlen invalid");

    return optval;
}

void modbus_tcp_client::connect_to_server(const std::string& host,
                            const std::string& service,
                            net::ip_protocol_version ip_version,
                            milliseconds timeout) {

    const char* service_;

    if (service.empty())
        service_ = pimpl->use_tls ? secure_server_default_port
                                  : server_default_port;
    else
        service_ = service.c_str();

    auto endpoints =
        net::resolve_endpoint(host.c_str(), service_, ip_version,
                         net::endpoint_usage::active_open);

    for (const auto& ep : endpoints) {
        int fd;

        if ((fd = socket(ep.family, ep.socktype | SOCK_CLOEXEC | SOCK_NONBLOCK,
                        ep.protocol)) == -1)
            throw system_error(errno, "socket");

        unique_fd ufd(fd);

        int on = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1)
            throw system_error(errno, "setsockopt TCP_NODELAY");

        int res = try_connect(fd, ep.addr.get(), ep.addrlen, timeout);

        if (res == 0) {

            pimpl->fd = std::move(ufd);
            return;
        }
        auto addr = net::to_endpoint_addr(ep.addr.get(), ep.addrlen);
        log::error("failed to connect to [{}]:{}: {}", addr.host, addr.service,
                   std::make_error_code(static_cast<std::errc>(res))
                       .message());
    }
    throw mboxid_error(errc::active_open_error,
                       "failed to connect to [" + host + "]:"  + service_);
}

} // namespace mboxid
