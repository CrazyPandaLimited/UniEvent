#include <xs/unievent.h>
#include "test.h"
#include <panda/unievent/Loop.h>
#include <panda/unievent/Resolver.h>

MODULE = MyTest                PACKAGE = MyTest
PROTOTYPES: DISABLE

void core_dump () { abort(); }

void variate_ssl (bool val) { variation.ssl = val; }

void variate_socks (bool val) { variation.socks = val; }

void variate_socks_url (string val) { variation.socks_url = val; }

void variate_buf (bool val) { variation.buf = val; }

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
