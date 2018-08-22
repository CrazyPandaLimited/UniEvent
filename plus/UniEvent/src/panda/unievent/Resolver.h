#pragma once

#include <panda/string_view.h>
#include <panda/unievent/global.h>
#include <panda/unievent/Loop.h>
#include <panda/unievent/Request.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/ResolveFunction.h>


namespace panda { namespace unievent {

class TCP;
class Resolver : public CancelableRequest, public AllocatedObject<Resolver, true> {
public:
    using resolve_fptr = void(Resolver* req, addrinfo* res, const ResolveError& err);
    using resolve_fn = function<resolve_fptr>;
    
    CallbackDispatcher<resolve_fptr> resolve_event_compat;
    CallbackDispatcher<ResolveFunctionPlain> resolve_event;

    ~Resolver();

    Resolver (Loop* loop = Loop::default_loop());

    virtual void resolve_compat (std::string_view node, std::string_view service = std::string_view(), 
            const addrinfo* hints = nullptr, resolve_fn callback = nullptr); 
    
    virtual void resolve (std::string_view node, std::string_view service = std::string_view(), 
            const addrinfo* hints = nullptr, ResolveFunction callback = nullptr); 

    static void free(addrinfo* ai);

    void call_on_resolve(addrinfo* res, const ResolveError& err);

protected:
    virtual void on_resolve (addrinfo* res, const ResolveError& err);

private:
    static void uvx_on_resolve (uv_getaddrinfo_t* req, int status, addrinfo* res);

public:
    //ConnectRequest* connect_request;
    TCP*            handle;

protected:
    uv_getaddrinfo_t uvr;
};

using ResolverSP = iptr<Resolver>;

}}

