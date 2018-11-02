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

bool variate_buf (bool val = false) {
    if (items) variation.buf = val;
    RETVAL = variation.buf;
}

void _benchmark_regular_resolver () { 
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver);
   
    for (auto i=0; i<1000; i++) {
        bool called = false;                                                          
        ResolveRequestSP request = resolver->resolve(loop.get(), 
                "localhost", 
                "80", 
                nullptr,
                [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                    called = true;
                });
    }
    
    loop->run();
}

void _benchmark_cached_resolver () { 
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver);
   
    // cache first 
    bool called = false;                                                          
    ResolveRequestSP request = resolver->resolve(loop.get(), 
            "localhost", 
            "80", 
            nullptr, 
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
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
                [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                    called = true;
                });
    }
    
    loop->run();
}                                                                                                                                                        
