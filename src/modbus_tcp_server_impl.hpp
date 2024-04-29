// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIBMBOXID_MODBUS_TCP_SERVER_IMPL_HPP
#define LIBMBOXID_MODBUS_TCP_SERVER_IMPL_HPP

#include <thread>
#include <vector>
#include <deque>
#include <list>
#include <variant>
#include <functional>
#include <chrono>
#include <poll.h>
#include <sys/eventfd.h>
#include <mboxid/modbus_tcp_server.hpp>
#include "unique_fd.hpp"
#include "network_private.hpp"

namespace mboxid {

class modbus_tcp_server::impl {
public:
    impl();
    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(impl&&) = delete;

    ~impl();

    void set_server_addr(std::string_view host, std::string_view service,
                         net::ip_protocol_version ip_version);

    void set_backend(std::unique_ptr<backend_connector> backend);

    void run();
    void shutdown();
    void close_client_connection(client_id id);
    void set_idle_timeout(milliseconds to);
    void set_request_complete_timeout(milliseconds to);

private:
    using timestamp = std::chrono::time_point<std::chrono::steady_clock>;
    static constexpr timestamp never = timestamp::max();

    struct cmd_stop { };

    struct cmd_close_connection { client_id id; };

    using cmd_queue_entry = std::variant<cmd_stop, cmd_close_connection>;

    // set of file descriptors to monitor
    struct monitor_set {
        std::vector<struct pollfd> fds;
        std::vector<std::function<void(int fd, unsigned events)>> on_ready;
    };

    struct client_control_block;

    bool stop_fl = false;

    unique_fd cmd_event_fd;
    std::vector<unique_fd> listen_fds;

    std::mutex cmd_queue_mutex;
    std::deque<cmd_queue_entry> cmd_queue;
    net::endpoint_addr own_addr;
    std::list<std::unique_ptr<client_control_block>> clients;
    std::unique_ptr<backend_connector> backend;
    timestamp ts_next_backend_ticker;

    milliseconds idle_timeout = no_timeout;
    milliseconds request_complete_timeout = no_timeout;

    void trigger_command_processing();
    int calc_poll_timeout();
    monitor_set build_monitor_set();
    void process_commands(int fd, unsigned events);
    void passive_open();
    void establish_connection(int fd, unsigned events);
    client_control_block* find_client_by_fd(int fd);
    void close_client_by_id(client_id id);
    void reset_client_state(client_control_block* client);
    timestamp determine_deadline(milliseconds to);
    bool receive_request(client_control_block* client);
    void execute_request(client_control_block* client);
    void handle_request(int fd, unsigned events);
    void send_response(int fd, unsigned events);

    void execute_pending_tasks();

    bool use_tls() { return false; }
};

} // namespace mboxid

#endif // LIBMBOXID_MODBUS_TCP_SERVER_IMPL_HPP
