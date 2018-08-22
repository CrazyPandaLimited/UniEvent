#pragma once
#include <panda/unievent/Request.h>
#include <panda/unievent/Resolver.h>
#include <panda/unievent/ResolveFunction.h>

namespace panda { namespace unievent {

class Handle;
class Stream;
class TCP;
class Pipe;

class CommandBase {
public:
    enum class Type {UNKNOWN, CLOSE_DELETE, CLOSE_REINIT, CONNECT, CONNECT_PIPE, WRITE, SHUTDOWN, USER_CALLBACK, RESOLVE};
    CommandBase* next;
    Type         type;
    Handle*      handle;
    virtual void run    () = 0;
    virtual void cancel () = 0;
    virtual ~CommandBase () {}
protected:
    CommandBase () : next(nullptr), type(Type::UNKNOWN), handle(nullptr) {}
};


class CommandCloseDelete : public CommandBase, public AllocatedObject<CommandCloseDelete> {
public:
    CommandCloseDelete  () { type = Type::CLOSE_DELETE; }
    void cancel () override;
    void run    () override;
};


class CommandCloseReinit : public CommandBase, public AllocatedObject<CommandCloseReinit> {
public:
    CommandCloseReinit  () { type = Type::CLOSE_REINIT; }
    void cancel () override;
    void run    () override;
};


class CommandConnect : public CommandBase {
public:
    void cancel () override;
    bool is_reconnect () const { return req->is_reconnect; }

protected:
    TCP* tcp;
    ConnectRequest* req;

    CommandConnect (TCP* tcp, ConnectRequest* req) : tcp(tcp), req(req) {
        type = Type::CONNECT;
        req->retain();
    }
    virtual ~CommandConnect ();
};


class CommandConnectPipe : public CommandBase {
public:
    CommandConnectPipe (Pipe* pipe, const string& name, ConnectRequest* req) : pipe(pipe), req(req) {
        type = Type::CONNECT_PIPE;
        _name = name;
        req->retain();
    }
    
    virtual ~CommandConnectPipe ();

    void run    () override;
    void cancel () override;
    
protected:
    Pipe* pipe;
    ConnectRequest* req;

private:
    string _name;
};


class CommandConnectSockaddr : public CommandConnect, public AllocatedObject<CommandConnectSockaddr> {
public:
    CommandConnectSockaddr (TCP* tcp, const sockaddr* sa, ConnectRequest* req) : CommandConnect(tcp, req) {
        if (sa->sa_family == AF_INET) sa4 = *((sockaddr_in*)sa);
        else sa6 = *((sockaddr_in6*)sa);
    }

    void run () override;

private:
    union {
        sockaddr_in  sa4;
        sockaddr_in6 sa6;
    };
};


class CommandConnectHost : public CommandConnect, public AllocatedObject<CommandConnectHost> {
public:
    CommandConnectHost (TCP* tcp, const string& host, const string& service, const addrinfo* hints, ConnectRequest* req, bool use_cached_resolver)
        : CommandConnect(tcp, req), has_hints(false)
    {
        this->host    = host;
        this->service = service;
        if (hints) {
            this->hints = *hints;
            has_hints   = true;
        }
        this->use_cached_resolver = use_cached_resolver;
    }

    void run () override;

private:
    string           host;
    string           service;
    addrinfo         hints;
    bool             has_hints;
    bool             use_cached_resolver;
};

class CommandResolveHost : public CommandBase, public AllocatedObject<CommandResolveHost> {
public:
    CommandResolveHost(TCP* stream, const string& host, const string& service, const addrinfo* hints, ResolveFunction callback,
                       bool use_cached_resolver)
            : stream_(stream)
            , host_(host)
            , service_(service)
            , callback_(callback)
            , use_cached_resolver_(use_cached_resolver) {
        type = Type::RESOLVE;
        if (hints) {
            hints_     = *hints;
            has_hints_ = true;
        }
    }

    void run() override;
    void cancel() override;

private:
    TCP*            stream_;
    string          host_;
    string          service_;
    addrinfo        hints_;
    bool            has_hints_;
    ResolveFunction callback_;
    bool            use_cached_resolver_;
};

class CommandWrite : public CommandBase, public AllocatedObject<CommandWrite> {
public:
    CommandWrite (Stream* stream, WriteRequest* req) : stream(stream), req(req) {
        type = Type::WRITE;
        req->retain();
    }

    void run    () override;
    void cancel () override;

    virtual ~CommandWrite ();

private:
    Stream* stream;
    WriteRequest* req;
};


class CommandShutdown : public CommandBase, public AllocatedObject<CommandShutdown> {
public:
    CommandShutdown (Stream* stream, ShutdownRequest* req) : stream(stream), req(req) {
        type = Type::SHUTDOWN;
        req->retain();
    }

    void run    () override;
    void cancel () override;

    virtual ~CommandShutdown ();

private:
    Stream* stream;
    ShutdownRequest* req;
};


class CommandCallback : public CommandBase, public AllocatedObject<CommandCallback> {
public:
    typedef void (*command_cb) (Handle* handle);

    CommandCallback (command_cb cb) : cb(cb) {}

    void run    () override;
    void cancel () override;
private:
    command_cb cb;

};

}}
