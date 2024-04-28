// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#include <limits>
#include <span>
#include <ranges>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "error_private.hpp"
#include "logger_private.hpp"
#include "crc32.h"
#include "modbus_protocol_common.hpp"
#include "modbus_protocol_server.hpp"
#include "modbus_tcp_server_impl.hpp"

namespace mboxid {

using std::uint8_t;
using std::size_t;

constexpr const char* server_default_port = "502";
constexpr const char* secure_server_default_port = "802";
constexpr int backlog{5};

constexpr std::chrono::milliseconds backend_ticker_period{1000};

struct modbus_tcp_server::impl::client_control_block {
    client_id id = 0;
    unique_fd fd;
    net::endpoint_addr addr;

    uint8_t req_buf[max_adu_size];
    uint8_t rsp_buf[max_adu_size];
    bool req_header_parsed = false;
    mbap_header req_header;

    std::span<uint8_t> req;
    std::span<const uint8_t> rsp;
};

/// Returns timestamp representing the current point in time.
static auto now() { return std::chrono::steady_clock::now(); }

modbus_tcp_server::impl::impl()
    : backend(std::make_unique<backend_connector>()),
      ts_next_backend_ticker(now() + backend_ticker_period)
{
    if (auto fd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE); fd == -1)
        throw system_error(errno, "eventfd");
    else
        cmd_event_fd.reset(fd);
}

modbus_tcp_server::impl::~impl() = default;

void modbus_tcp_server::impl::set_server_addr(std::string_view host,
                                        std::string_view service,
                                        net::ip_protocol_version ip_version) {
    own_addr.host = host;
    own_addr.service = service;
    own_addr.ip_version = ip_version;
}

void modbus_tcp_server::impl::set_backend(
        std::unique_ptr<backend_connector> backend) {
    validate_argument(backend.get(), "set_backend");
    this->backend = std::move(backend);
}

void modbus_tcp_server::impl::run() {
    passive_open();

    while (!stop_fl) {
        auto set = build_monitor_set();
        auto to = calc_poll_timeout();

        auto res = TEMP_FAILURE_RETRY(poll(set.fds.data(), set.fds.size(), to));
        if (res == -1)
            throw system_error(errno, "poll");
        else if (res > 0) {
            for (size_t i = 0; i < set.fds.size(); ++i) {
                if (set.fds[i].revents)
                    set.on_ready[i](set.fds[i].fd, set.fds[i].revents);
            }
        }
        execute_pending_tasks();
    }
}

void modbus_tcp_server::impl::shutdown() {
    {
        std::lock_guard m(cmd_queue_mutex);
        cmd_queue.emplace_back(cmd_stop());
    }
    trigger_command_processing();
}

void modbus_tcp_server::impl::close_client_connection(client_id id) {
    {
        std::lock_guard m(cmd_queue_mutex);
        cmd_queue.emplace_back(cmd_close_connection{id});
    }
    trigger_command_processing();
}

void modbus_tcp_server::impl::trigger_command_processing() {
    if (eventfd_write(cmd_event_fd.get(), 1) == -1)
        throw system_error(errno, "eventfd_write");
}

int modbus_tcp_server::impl::calc_poll_timeout() {
    using namespace std::chrono;

    auto to = milliseconds{std::numeric_limits<int>::max()};
    auto now_ = now();

    if (now_ >= ts_next_backend_ticker)
        return 0;
    to = std::min(to, ceil<milliseconds>(ts_next_backend_ticker - now_));

    return static_cast<int>(to.count());
}

auto modbus_tcp_server::impl::build_monitor_set() -> monitor_set {
    monitor_set set;

    auto n_fds = 1 + listen_fds.size();
    set.fds.reserve(n_fds);
    set.on_ready.reserve(n_fds);

    struct pollfd pollfd = { .fd = -1, .events = POLLIN, .revents = 0};

    pollfd.fd = cmd_event_fd.get();
    set.fds.push_back(pollfd);
    set.on_ready.emplace_back(
        [this](int fd, unsigned events) {this->process_commands(fd, events);});

    for (const auto& fd : listen_fds) {
        pollfd.fd = fd.get();
        set.fds.push_back(pollfd);
        set.on_ready.emplace_back(
            [this](int fd, unsigned events) {
            this->establish_connection(fd, events) ;});
    }

    for (const auto& client : clients) {
        pollfd.fd = client->fd.get();
        if (client->rsp.empty()) {
            pollfd.events = POLLIN;
            set.on_ready.emplace_back([this](int fd, unsigned events) {
                this->handle_request(fd, events);
            });
        }
        else {
            pollfd.events = POLLOUT;
            set.on_ready.emplace_back([this](int fd, unsigned events) {
                this->send_response(fd, events);
            });

        }
        set.fds.push_back(pollfd);
    }

    return set;
}

static void validate_poll_events(const char* where, unsigned events,
                                 unsigned expected)
{
    using namespace std::string_literals;

    if (events & ~expected) {
        char hex[20];
        std::snprintf(hex, sizeof(hex), "0x%08x", (events & ~expected));
        std::string msg =  where + ": unexpected poll event(s) "s + hex;
        throw (mboxid_error(errc::logic_error, msg));
    }
    if (!(events & expected)) {
        char hex[20];
        std::snprintf(hex, sizeof(hex), "0x%08x", expected);
        std::string msg =  where + ": missing poll event(s) "s + hex;
        throw (mboxid_error(errc::logic_error, msg));
    }
}

void modbus_tcp_server::impl::process_commands(int fd, unsigned events) {
    validate_poll_events("process_commands", events, POLLIN);

    // consume trigger event
    eventfd_t cnt;
    if (eventfd_read(fd, &cnt) == -1)
        throw system_error(errno, "eventfd_read");

    // take ownership of queued commands
    std::deque<cmd_queue_entry> cmds;
    {
        std::lock_guard m(cmd_queue_mutex);
        std::swap(cmd_queue, cmds);
    }

    // process all queued commands
    for (const auto& cmd : cmds) {
        if (std::holds_alternative<cmd_stop>(cmd))
            stop_fl = true;
        else if (std::holds_alternative<cmd_close_connection>(cmd)) {
            close_client_by_id(std::get<cmd_close_connection>(cmd).id);
        }
        else
            throw mboxid_error(errc::logic_error, "process_commands");
    }
}

void modbus_tcp_server::impl::passive_open()
{
    const char* host = own_addr.host.empty() ? nullptr : own_addr.host.c_str();
    const char* service;

    if (own_addr.service.empty())
        service = use_tls() ? secure_server_default_port : server_default_port;
    else
        service = own_addr.service.c_str();

    auto endpoints = resolve_endpoint(host, service, own_addr.ip_version,
                                      net::endpoint_usage::passive_open);

    for (const auto& ep : endpoints) {
        unique_fd fd(socket(ep.family,
                            ep.socktype | SOCK_CLOEXEC | SOCK_NONBLOCK,
                            ep.protocol));
        auto fd_ = fd.get();

        if (fd_ == -1)
            throw system_error(errno, "socket");

        int on = 1;
        if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
            throw system_error(errno, "setsockopt SO_REUSEADDR");

        if (bind(fd_, ep.addr.get(), ep.addrlen) == -1) {
            auto msg = std::error_code(errno, std::system_category()).message();
            auto ep_addr = net::to_endpoint_addr(ep.addr.get(), ep.addrlen);
            log::error("bind to [{}]:{} failed: {}",
                       ep_addr.host, ep_addr.service, msg);
            continue;
        }

        if (listen(fd_, backlog) == -1) {
            auto msg = std::error_code(errno, std::system_category()).message();
            auto ep_addr = net::to_endpoint_addr(ep.addr.get(), ep.addrlen);
            log::error("listen on [{}]:{} failed: {}",
                       ep_addr.host, ep_addr.service, msg);
            continue;
        }

        listen_fds.push_back(std::move(fd));
    }

    if (listen_fds.empty())
        throw mboxid_error(errc::passive_open_error,
                           "failed to bind to any interface");
}

static auto gen_client_id(int fd, const sockaddr* addr, socklen_t addrlen) {
    using client_id = modbus_tcp_server::client_id;
    client_id id;

    auto crc = crc_finalize(crc_update(crc_init(), addr, addrlen));

    id = static_cast<client_id>(fd) << 32 | crc;
    return id;
}

void modbus_tcp_server::impl::establish_connection(int fd, unsigned events) {
    validate_poll_events("establish_connection", events, POLLIN);

    struct sockaddr_storage addr{};
    socklen_t addrlen = sizeof(addr);
    auto sa = reinterpret_cast<struct sockaddr*>(&addr);

    unique_fd conn_fd(TEMP_FAILURE_RETRY(accept4(fd, sa, &addrlen,
                                        SOCK_NONBLOCK | SOCK_CLOEXEC)));

    auto conn_fd_ = conn_fd.get();

    if (conn_fd_ == -1) {
        switch (errno) {
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK: [[falltrough]];
#endif
        case EAGAIN:
            return;
        case ECONNABORTED: [[fallthrough]];
        case ETIMEDOUT:
            log::error("establich_connection aborted prematurely: {}",
                    std::error_code(errno, std::system_category()).message());
            return;
        default:
            throw system_error(errno, "accept4");
        }
    }

    auto client = std::make_unique<client_control_block>();
    client->id = gen_client_id(conn_fd_, sa, addrlen);
    client->fd = std::move(conn_fd);
    client->addr = net::to_endpoint_addr(sa, addrlen);

    int on = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1)
        throw system_error(errno, "setsockopt TCP_NODELAY");

    auto authorized = backend->authorize(client->id, client->addr,
                                         sa, addrlen);

    log::info("client(id={:#x}) connecting from [{}]:{} {}",
        client->id, client->addr.host, client->addr.service,
        authorized ? "accepted" : "denied");

    if (authorized)
        clients.push_back(std::move(client));
}

auto modbus_tcp_server::impl::find_client_by_fd(int fd) -> client_control_block*
{
    auto res = std::ranges::find_if(clients,
                                    [fd](const auto& client) {
                                        return client->fd.get() == fd; });

    if (res == clients.end()) {
        log::warning("find_client_by_id(): client(fd={}) not found", fd);
        return nullptr;
    }
    return res->get();
}

void modbus_tcp_server::impl::close_client_by_id(client_id id)
{
    auto cnt = std::erase_if(clients,
                             [id](auto& client) {
                                 return client->id == id; });
    if (cnt) {
        backend->disconnect(id);
        log::info("client(id={:#x}) disconnected", id);
    }
    else
        log::warning("close_client_by_id(): client(id={:#x}) not found", id);
}

void modbus_tcp_server::impl::reset_client_state(client_control_block* client) {
    client->req_header_parsed = false;
    client->req = std::span<uint8_t>();
    client->rsp = std::span<uint8_t>();
}

bool modbus_tcp_server::impl::receive_request(client_control_block* client) {
    int fd = client->fd.get();
    size_t total = client->req.size();
    size_t left;
    ssize_t cnt;

    if (total < mbap_header_size)
        left = mbap_header_size - total;
    else {
        if (!client->req_header_parsed) {
            parse_mbap_header(client->req, client->req_header);
            client->req_header_parsed = true;
        }
        left = get_adu_size(client->req_header) - total;
    }

   cnt = TEMP_FAILURE_RETRY(read(fd, &client->req_buf[total], left));
    if (cnt == -1) {
        switch (errno) {
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK: [[falltrough]];
#endif
        case EAGAIN:
            return false;
        default:
            throw system_error(errno, "read()");
        }
    }
    else if (cnt == 0) {
        close_client_by_id(client->id);
        return false;
    }

    total += cnt;
    left -= cnt;
    client->req = std::span(client->req_buf, total);
    return ((total > mbap_header_size) && (left == 0));
}

void modbus_tcp_server::impl::execute_request(client_control_block* client) {
    std::span rsp{client->rsp_buf};
    auto rsp_header = client->req_header;

    size_t cnt = server_engine(*backend, client->req.subspan(mbap_header_size),
                               rsp.subspan(mbap_header_size));
    rsp_header.length = cnt + sizeof(rsp_header.unit_id);
    cnt += serialize_mbap_header(rsp.subspan(0, mbap_header_size), rsp_header);

    client->rsp = rsp.subspan(0, cnt);
}

void modbus_tcp_server::impl::handle_request(int fd, unsigned int events) {
    validate_poll_events("handle_request", events, POLLHUP | POLLERR | POLLIN);

    auto client = find_client_by_fd(fd);
    if (!client)
        return;

    if (events & (POLLHUP | POLLERR)) {
        close_client_by_id(client->id);
        return;
    }

    try {
        if (receive_request(client)) {
            execute_request(client);
            backend->alive(client->id);
        }
    }
    catch (const mboxid_error& e) {
        if (e.code() == errc::parse_error) {
            log::error("client(id={:#x}) request: {}", client->id, e.what ());
            // As TCP provides reliable data transfer we consider every
            // parse error as serious failure. We close the connection to
            // discard possible corrupted data in-flight and force the client
            // to reconnect.
            close_client_by_id(client->id);
        }
        else
            throw;
    }
}

void modbus_tcp_server::impl::send_response(int fd, unsigned int events) {
    validate_poll_events("receive_request", events,
                         POLLHUP | POLLERR | POLLOUT);
    auto client = find_client_by_fd(fd);
    if (!client)
        return;

    if (events & (POLLHUP | POLLERR)) {
        close_client_by_id(client->id);
        return;
    }

    auto& rsp = client->rsp;
    ssize_t cnt;
    cnt = TEMP_FAILURE_RETRY(send(fd, rsp.data(), rsp.size(), MSG_NOSIGNAL));

    if (cnt == -1) {
        switch (errno) {
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK: [[falltrough]];
#endif
        case EAGAIN:
            return;
        case EPIPE: [[fallthrough]];
        case ECONNRESET:
            close_client_by_id(client->id);
            return;
        default:
            throw system_error(errno, "send()");
        }
    }

    rsp = rsp.subspan(cnt);
    if (rsp.empty())
        reset_client_state(client);
}

void modbus_tcp_server::impl::execute_pending_tasks() {
    auto now_ = now();

    if (now_ >= ts_next_backend_ticker) {
        backend->ticker();
        ts_next_backend_ticker = now_ + backend_ticker_period;
    }
}


} // namespace mboxid