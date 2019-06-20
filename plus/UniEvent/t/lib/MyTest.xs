#include <xs/unievent.h>
#include "test.h"

MODULE = MyTest                PACKAGE = MyTest
PROTOTYPES: DISABLE

void core_dump () { abort(); }

bool variate_ssl (bool val = false) {
    if (items) variation.ssl = val;
    RETVAL = variation.ssl;
}

bool variate_buf (bool val = false) {
    if (items) variation.buf = val;
    RETVAL = variation.buf;
}

void _benchmark_simple_resolver () { 
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));
    
    for (auto i = 0; i < 1000; i++) {
        bool called = false;
        resolver->resolve()->node("localhost")->use_cache(false)->on_resolve([&](const AddrInfo&, const CodeError&, const Resolver::RequestSP&) {
            called = true;
        })->run();
    }
    
    loop->run();
}

void _benchmark_cached_resolver () { 
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));
   
    // put it into cache first 
    bool called = false;                                                          
    resolver->resolve("localhost", [&](const AddrInfo&, const CodeError&, const Resolver::RequestSP&) {
        called = true;
    });
    
    // will resolve and cache here, loop will exit as there are no pending requests
    loop->run();

    // resolve gets address from cache 
    for (auto i = 0; i < 99999; i++) {
        bool called = false;                                                          
        resolver->resolve("localhost", [&](const AddrInfo&, const CodeError&, const Resolver::RequestSP&) {
            called = true;
        });
    }
    
    loop->run();
}

void _benchmark_timer_start_stop (LoopSP loop, int tmt, int cnt) {
    TimerSP timer(new Timer(loop));
    for (int i = 0; i < cnt; ++i) {
        timer->start(tmt);
        timer->stop();
    }
}

void _benchmark_loop_update_time (int cnt) {
    LoopSP loop(new Loop);
    for (int i = 0; i < cnt; ++i) loop->update_time();
}

void _bench_delay_add_rm (int cnt) {
    auto loop = Loop::default_loop();
    for (int i = 0; i < cnt; ++i) {
        auto ret = loop->delay([]{});
        loop->cancel_delay(ret);
    }
}

void _bench_loop_iter (int cnt) {
    auto l = Loop::default_loop();
    for (int i = 0; i < cnt; ++i) l->run_nowait();
}