#pragma once
#include <panda/unievent/Request.h>
#include <panda/unievent/ResolveFunction.h>

namespace panda { namespace unievent {

class Handle;
class Stream;
class TCP;
class Pipe;

class CommandBase {
public:
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

class TCPConnectRequest;
class CommandConnect : public CommandBase {
public:
    CommandConnect (TCP* tcp, TCPConnectRequest* tcp_connect_request);
    void cancel () override;
    void run    () override;
    bool is_reconnect () const;

protected:
    TCP* tcp;
    TCPConnectRequest* tcp_connect_request;

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
