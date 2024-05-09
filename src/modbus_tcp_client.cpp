// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <poll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <memory>
#include <mboxid/modbus_tcp_client.hpp>
#include "unique_fd.hpp"
#include "network_private.hpp"
#include "logger_private.hpp"
#include "error_private.hpp"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_client.hpp"

namespace mboxid {

using timestamp = std::chrono::time_point<std::chrono::steady_clock>;
constexpr auto never = timestamp::max();

constexpr unsigned min_unit_id = 0U;
constexpr unsigned max_unit_id = 0xffU;

struct modbus_tcp_client::context {
    unique_fd fd;
    bool use_tls = false;
    milliseconds timeout = no_timeout;
    uint8_t pdu[max_pdu_size]{};
    uint16_t transaction_id = 0;
    uint8_t unit_id = 0;
};

using context = modbus_tcp_client::context;

/// Returns timestamp representing the current point in time.
static auto now() { return std::chrono::steady_clock::now(); }

modbus_tcp_client::modbus_tcp_client() : ctx(std::make_unique<context>()) {}

modbus_tcp_client::~modbus_tcp_client() = default;

 /**
  * Try to connect to the sever within specified duration.
  *
  * @param fd Socket file descriptor (non-blocking).
  * @param addr Address of the server.
  * @param addrlen Size of the \a addr argument.
  * @param timeout no_timeout to disable the timeout, otherwise the maximum
  *     allowed duration to block.
  *
  * @return 0 on success, otherwise a POSIX error number (@see man 3 errno).
  *
  * \pre The socket file descriptor \a fd must be set to non-blocking.
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

static void set_socket_blocking(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
        throw system_error(errno, "fcntl: F_GETFL");
    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
        throw system_error(errno, "fcntl: F_SETFL");
}

void modbus_tcp_client::connect_to_server(const std::string& host,
                            const std::string& service,
                            net::ip_protocol_version ip_version,
                            milliseconds timeout) {
    const char* service_;

    if (service.empty())
        service_ =
            ctx->use_tls ? secure_server_default_port
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
            // send() shall block if necessary so that there is no need to
            // deal with EAGAIN/EWOULDBLOCK (which should never happen anyway).
            // For recv() the per-call option MSG_DONTWAIT is used.
            set_socket_blocking(fd);
            ctx->fd = std::move(ufd);
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

void modbus_tcp_client::set_response_timeout(milliseconds timeout) {
    ctx->timeout = timeout;
}

void modbus_tcp_client::set_unit_id(unsigned id) {
    validate_argument(id, min_unit_id, max_unit_id, "set_unit_id");
    ctx->unit_id = id;
}

static void send_frame(int fd, const mbap_header& mbap,
                     std::span<const uint8_t> req)
{
    uint8_t mbap_buf[mbap_header_size];

    serialize_mbap_header(mbap_buf, mbap);

    struct iovec iov[] = {
        { .iov_base = mbap_buf, .iov_len = sizeof(mbap_buf) },
        { .iov_base = const_cast<uint8_t*>(req.data()), .iov_len = req.size() },
    };

    struct msghdr msg{};
    msg.msg_iov = iov;
    msg.msg_iovlen = sizeof(iov) / sizeof(*iov);

    auto cnt = TEMP_FAILURE_RETRY(sendmsg(fd, &msg, MSG_NOSIGNAL));

    if (cnt == -1) {
        switch (errno) {
        case EPIPE: [[fallthrough]];
        case ECONNRESET:
            throw mboxid_error(errc::connection_closed);
        default:
            throw system_error(errno, "send()");
        }
    }
}

static void receive_all(int fd, std::span<uint8_t> buf, size_t cnt,
                        timestamp deadline) {
    expects(buf.size() >= cnt, "buffer too small");

    size_t total = 0;
    ssize_t chunk_cnt;

    do {
        auto now_ = now();
        if (now_ > deadline)
            throw mboxid_error(errc::timeout, "receive_all");

        struct pollfd pollfd = { .fd = fd, .events = POLLIN, .revents = 0};
        int to = (deadline == never)
                     ? -1 : static_cast<int>((deadline - now_) .count());

        int res = TEMP_FAILURE_RETRY(poll(&pollfd, 1, to));

        if (res < 0)
            throw system_error(errno, "poll");
        else if (res == 0)
            throw mboxid_error(errc::timeout, "receive_all");
        else if ((res != 1) ||
                 !(pollfd.revents & (POLLIN | POLLHUP | POLLERR)))
            throw mboxid_error(errc::logic_error, "receive_all: spurious poll");

        if (pollfd.revents & (POLLHUP | POLLERR))
            throw mboxid_error(errc::connection_closed, "receive_all");

        chunk_cnt = TEMP_FAILURE_RETRY(
                        recv(fd, &buf[total], cnt - total, MSG_DONTWAIT));

        if (chunk_cnt < 0) {
            switch (errno) {
#if EAGAIN != EWOULDBLOCK
            case EWOULDBLOCK: [[falltrough]];
#endif
            case EAGAIN:
                continue;
            default:
                throw system_error(errno, "recv");
            }
        }
        else if (chunk_cnt == 0)
            throw mboxid_error(errc::connection_closed, "receive_all");

        total += chunk_cnt;
    } while (total < cnt);
}


static void receive_frame(int fd, mbap_header& mbap, std::span<uint8_t>& pdu,
                          milliseconds timeout) {
    timestamp deadline = (timeout == no_timeout) ? never : now() + timeout;
    uint8_t mbap_buf[mbap_header_size];

    receive_all(fd, mbap_buf, sizeof(mbap_buf), deadline);
    parse_mbap_header(mbap_buf, mbap);
    auto cnt = get_pdu_size(mbap);
    receive_all(fd, pdu, cnt, deadline);
    pdu = pdu.subspan(0, cnt);
};

static std::span<const uint8_t> send_receive_pdu(context* ctx,
                                             std::span<const uint8_t> req) {

    mbap_header mbap = {
        .transaction_id = ++ctx->transaction_id,
        .protocol_id = 0,
        .length = 0,
        .unit_id = ctx->unit_id };
    set_pdu_size(mbap, req.size());

    send_frame(ctx->fd.get(), mbap, req);

    std::span<uint8_t> rsp{ctx->pdu};

    receive_frame(ctx->fd.get(), mbap, rsp, ctx->timeout);

    if ((mbap.transaction_id != ctx->transaction_id) ||
        (mbap.unit_id != ctx->unit_id)) {
        log::error("invalid response header (transaction_id = {}, unit_id = {}",
                   mbap.transaction_id, max_unit_id);
        throw(mboxid_error(errc::parse_error, "mbap header mismatch"));
    }

    return rsp;
}

std::vector<bool> modbus_tcp_client::read_coils(unsigned addr, size_t cnt)
{
    std::span<uint8_t> req{ctx->pdu};

    // serialize request
    auto len = serialize_read_coils_request(ctx->pdu, addr, cnt);
    auto rsp = send_receive_pdu(&*ctx, req.subspan(0, len));

    std::vector<bool> coils;
    parse_read_coils_response(rsp, coils, cnt);
    return coils;
}

} // namespace mboxid
