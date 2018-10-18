#include <xs/unievent.h>
#include "test.h"
#include <memory>
#include <panda/log.h>
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <panda/unievent/Loop.h>
#include <panda/unievent/Resolver.h>

MODULE = MyTest                PACKAGE = MyTest
PROTOTYPES: DISABLE

void TEST_SSL   (bool val) { TEST_SSL   = val; }

void TEST_SOCKS (bool val) { TEST_SOCKS = val; }

void TEST_BUF   (bool val) { TEST_BUF   = val; }

void test_tls () {
//    Thread* thr = new Thread();
//    thr->create_callback(tcb);
//    thr->create();
//    printf("MAIN THR: CREATED\n");
//    sleep(10);
//    printf("MAIN THR: JOINING...\n");
//    thr->join();
//    printf("MAIN THR: JOINED!\n");
}

void ttt () {
    for (int i = 0; i < 1000000; ++i) {
    }
}

uint64_t test_dynamic_cast (Handle*) {
    RETVAL = 0;
}

uint64_t test_dyn_cast (Handle*) {
    RETVAL = 0;
}

void _benchmark_regular_resolver () { 
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));
   
    for (auto i=0; i<1000; i++) {
        bool called = false;                                                          
        ResolveRequestSP request = resolver->resolve(loop.get(), 
                "localhost", 
                "80", 
                nullptr,
                [&](ResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                    called = true;
                });
    }
    
    loop->run();
}

void _benchmark_cached_resolver () { 
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop));
   
    // cache first 
    bool called = false;                                                          
    ResolveRequestSP request = resolver->resolve(loop.get(), 
            "localhost", 
            "80", 
            nullptr, 
            [&](ResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                called = true;
            });
    
    // will resolve and cache here, loop will exit as there are no pending requests 
    loop->run();

    // resolve from cache 
    for(auto i=0; i<99999; i++) {
        bool called = false;                                                          
        ResolveRequestSP request = resolver->resolve(loop.get(), 
                "localhost", 
                "80", 
                nullptr,
                [&](ResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                    called = true;
                });
    }
    
    loop->run();
}                                                                                                                                                        
