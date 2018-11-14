#pragma once

#include "Fwd.h"
#include "Request.h"

namespace panda { namespace unievent {

struct CommandBase {
    enum class Type {UNKNOWN, CLOSE_DELETE, CLOSE_REINIT, CONNECT, CONNECT_PIPE, WRITE, SHUTDOWN, USER_CALLBACK};
    CommandBase* next;
    Type         type;
    Handle*      handle;
    virtual void run    () = 0;
    virtual void cancel () = 0;
    virtual ~CommandBase () {}
protected:
    CommandBase () : next(nullptr), type(Type::UNKNOWN), handle(nullptr) {}
};


struct CommandCloseDelete : CommandBase, AllocatedObject<CommandCloseDelete> {
    CommandCloseDelete  () { type = Type::CLOSE_DELETE; }
    void cancel () override;
    void run    () override;
};


struct CommandCloseReinit : CommandBase, AllocatedObject<CommandCloseReinit> {
    CommandCloseReinit  () { type = Type::CLOSE_REINIT; }
    void cancel () override;
    void run    () override;
};

struct CommandConnect : CommandBase, AllocatedObject<CommandConnect> {
    CommandConnect (TCP* tcp, TCPConnectRequest* tcp_connect_request);
    void cancel () override;
    void run    () override;
    bool is_reconnect () const;

protected:
    TCP* tcp;
    TCPConnectRequest* tcp_connect_request;

    virtual ~CommandConnect ();
};


struct CommandConnectPipe : CommandBase, AllocatedObject<CommandConnectPipe> {
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


struct CommandWrite : CommandBase, AllocatedObject<CommandWrite> {
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


struct CommandShutdown : CommandBase, AllocatedObject<CommandShutdown> {
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


struct CommandCallback : CommandBase, AllocatedObject<CommandCallback> {
    typedef void (*command_cb) (Handle* handle);

    CommandCallback (command_cb cb) : cb(cb) {}

    void run    () override;
    void cancel () override;
private:
    command_cb cb;

};

}}
