#pragma once

#include <panda/unievent/Loop.h>
#include <panda/unievent/TCP.h>

using namespace panda::unievent;

// from environment UNIEVENT_TEST_SSL
extern bool TEST_SSL;

// from environment UNIEVENT_TEST_SOCKS
extern bool TEST_SOCKS;

TCPSP make_basic_server(in_port_t port, Loop* loop);
TCPSP make_socks_server(in_port_t port, LoopSP loop);
TCPSP make_server(in_port_t port, Loop* loop);
TCPSP make_client(Loop* loop, bool cached_resolver = true);
// from environment PANDA_TEST_BUF
extern bool TEST_BUF;

SSL_CTX* get_ssl_ctx();

TimerSP read(StreamSP stream, Stream::read_fn callback, uint64_t timeout = 1000);
