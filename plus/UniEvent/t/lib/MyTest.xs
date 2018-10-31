#include <xs/unievent.h>
#include "test.h"
#include <panda/unievent/Loop.h>
#include <panda/unievent/Resolver.h>

MODULE = MyTest                PACKAGE = MyTest
PROTOTYPES: DISABLE

void core_dump () { abort(); }

bool variate_ssl (bool val = false) {
    if (items) variation.ssl = val;
    RETVAL = variation.ssl;
}

int variate_socks (int val = 0) {
    if (items) variation.socks = val;
    RETVAL = variation.socks;
}

bool variate_buf (bool val = false) {
    if (items) variation.buf = val;
    RETVAL = variation.buf;
}

string variate_socks_url (string val = "") {
    if (items) variation.socks_url = val;
    RETVAL = variation.socks_url;
}

void _benchmark_regular_resolver () { 
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));
   
    for (auto i=0; i<1000; i++) {
        bool called = false;                                                          
        resolver->resolve( 
                "localhost", 
                "80", 
                nullptr,
                [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*){
                    called = true;
                });
    }
    
    loop->run();
}

void _benchmark_cached_resolver () { 
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop));
   
    // put it into cache first 
    bool called = false;                                                          
    resolver->resolve( 
            "localhost", 
            "80", 
            nullptr, 
            [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*){
                called = true;
            });
    
    // will resolve and cache here, loop will exit as there are no pending requests 
    loop->run();

    // resolve gets address from cache 
    for(auto i=0; i<99999; i++) {
        bool called = false;                                                          
        resolver->resolve( 
                "localhost", 
                "80", 
                nullptr,
                [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*){
                    called = true;
                });
    }
    
    loop->run();
}                                                                                                                                                        
