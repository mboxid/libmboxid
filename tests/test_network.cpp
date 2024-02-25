// Copyright (c) 2024, Franz Hollerer.
// SPDX-License-Identifier: BSD-3-Clause
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logger_private.hpp"
#include "network_private.hpp"

using namespace mboxid;
using testing::AnyOf;

TEST(NetworkTest, ResolveEndpoint) {
    std::list<net::endpoint> endpoints;
    net::endpoint_addr saddr;
    const net::endpoint* ep;

    endpoints =
        resolve_endpoint("localhost", "", net::ip_protocol_version::any,
                         net::endpoint_usage::passive_open);
    EXPECT_EQ(endpoints.size(), 2);
    for (const auto& ep: endpoints) {
        saddr = net::to_endpoint_addr(ep.addr.get(), ep.addrlen, true);
        EXPECT_THAT(saddr.host, AnyOf("127.0.0.1", "::1"));
    }

    endpoints =
        resolve_endpoint("localhost", "", net::ip_protocol_version::v4,
                         net::endpoint_usage::passive_open);
    EXPECT_EQ(endpoints.size(), 1);
    ep = &endpoints.front();
    saddr = net::to_endpoint_addr(ep->addr.get(), ep->addrlen);
    EXPECT_EQ(saddr.host, "127.0.0.1");

    endpoints =
        resolve_endpoint("localhost", "", net::ip_protocol_version::v6,
                         net::endpoint_usage::passive_open);
    EXPECT_EQ(endpoints.size(), 1);
    ep = &endpoints.front();
    saddr = net::to_endpoint_addr(ep->addr.get(), ep->addrlen);
    EXPECT_EQ(saddr.host, "::1");
}