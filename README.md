# UniEvent

UniEvent - Object-oriented, fast and extendable event loop abstraction framework with C++ and Perl interfaces.

# Build and Install

UniEvent can be built using CMake. There are several dependencies. Make sure that [find_package](https://cmake.org/cmake/help/latest/command/find_package.html) can find it:

* [panda-lib](https://github.com/CrazyPandaLimited/panda-lib/tree/master/clib)
* [panda-net-sockaddr](https://github.com/CrazyPandaLimited/Net-SockAddr)
* [c-ares](https://github.com/c-ares/c-ares)
* [libuv](https://github.com/libuv/libuv)
* OpenSSL

OpenSSL doesn't have its own CMake package but CMake can find it if it is installed in system.
Also `libuv` can be installed in system and UniEvent finds it. To build with tests [Catch2](https://github.com/catchorg/Catch2) is also required.

The easiest way to use libuv is to clone all the repositories to a folder and add all of it using `add_subdirectory`:

```CMake
add_subdirectory(deps/panda-lib)
add_subdirectory(deps/net-sockaddr)
add_subdirectory(c-ares)
add_subdirectory(catch2)

add_subdirectory(unievent)

target_link_libraries(${PROJECT_NAME} PUBLIC unievent)
```

You can alse build all dependencies separetly. In this case make sure that CMAKE_PREFIX_PATH is set to correct folder.

By default UniEvent is a static library. To build a dynamic library set CMake variable `LIB_TYPE` to `dynamic`.

# Documentation

UniEvent is a cross-platform extendable object-oriented event loop framework. It's also an abstraction layer on top of event loop which provides engine-independent API.

UniEvent is designed to support multiple backends (libuv is the only implemented at the moment).

## Memory Management and Lifetime

All dynamic objects are managed using [panda::iptr](https://github.com/CrazyPandaLimited/panda-lib/blob/master/clib/doc/refcnt.md#iptr). Never delete objects manually because it can be references to them inside of the framework.

All the types that suppose to be used as heap allocated have private destrucor. It prevents any kind of lifetime errors.

Also there are aliases for all `iptr<Type>` to make usage shorter. They have `SP` suffix in the names (stands for smart pointer), e.g. LoopSP, TimerSP, TcpSP.

See more about handles lifetime in section [Holding Handles](#holding-handles).

## Loop

The heart of event programming is an event loop object. This object runs the loop and polls for all registered events (handles).

You can create as many loops as you wish but you can only run one loop at a time. Each loop only polls for events registred with this loop.

```cpp
LoopSP loop = new Loop();
TimerSP timer = new Timer(loop);
timer->once(1000, [loop](auto) {
    loop->stop();
});
loop->run();
```

By default, one loop (which is called main or default) is automatically created for you and is accessible as

```cpp
    Loop::default_loop();
```

All event handles (watchers) will register in default loop if you don't specify a loop in their constructor.

```cpp
TimerSP timer = new Timer(loop);
timer->once(1000, [](auto) { Loop::default_loop()->stop(); });
Loop::default_loop()->run();
```

When you run
```cpp
    loop->run();
```
The execution flow will not return until there are no more active handles in the loop or loop stop() is called from some callback.
```cpp
    loop->stop();
```

Each handle has a strong refence to it's event loop so that loop will not be destroyed until you loose all refs to it and all it's handles are destroyed.

The default loop is never destroyed.

## Handles (Watchers)

There are a number of different handle classes to watch for different events like timers, signals, tcp and udp handles, filesystem operations, and so on. You can find detailed description of each handle class in reference.

Each handle object binds to a specific loop upon creation. Re-binding a handle to a different loop object after creation is not supported. A handle will watch for events as soon as you run the loop it was bound to.

* [Timer handle](doc/timer.md) invokes callback periodically at some interval (may be fractional).
* [TCP handles](tcp.md) are used to represent both TCP streams and servers.
* [Signal handle](doc/signal.md) watches for signal events.
* [Idle handle](doc/idle.md) invokes callback when the loop is idle, i.e. there are no new events after polling for i/o, timer, etc. On each loop iteration.
* [Prepare handle](doc/prepare.md) will run the given callback once per loop iteration, right before polling for i/o.
* [Check handle](doc/check.md) will run the given callback once per loop iteration, right after polling for i/o.
* [Pipe handles](doc/pipe.md) provide an abstraction over streaming files on Unix (including local domain sockets, pipes, and FIFOs) and named pipes on Windows.
* [TTY](doc/tty.md) handles represent a stream for the console.
* [UDP](doc/udp.md) handles encapsulate UDP communication for both clients and servers.
* [Poll](doc/poll.md) handles are used to watch file descriptors for readability, writability and disconnection.  *NOTE: If you want to connect, write, and read from network connections or make a network server,byou'd better use more convenient and more efficient cross-platform `tcp`, `udp`, `pipe`, etc... handles. *
* [FS Event](doc/fsevent.md) handles allow the user to monitor a given path for changes, for example, if the file was renamed or there was a generic change in it.
* [FS Poll](doc/fspoll.md) handles allow the user to monitor a given path for changes. Unlike `FsEvent`, fs poll handles use `stat` to detect when a file has changed so they can work on file systems where fs event handles can’t.

## Events

All events are dispatched using [CallbackDispatcher](https://github.com/CrazyPandaLimited/panda-lib/blob/master/clib/doc/CallbackDispatcher.md). Usually there is a member called `event` on the handles that have only one event to dispatch. In other cases event-members hass suffix `_event` in names, e.g. `read_event`, `connect_event`, etc.

All the events has a source handle as a first argument. For example, `read_event` has first argument `StreamSP` that points to a stream that dispatched an event.

## EventListener

Instead of setting callbacks for each event type
```cpp
    tcp->read_event([](auto...){});
    tcp->write_event([](auto...){});
```

you can set an event listener object which can watch for all events types.

```cpp
struct MyListener : IStreamListener {
    virtual void     on_connect (const StreamSP&, const ErrorCode&, const ConnectRequestSP&);
    virtual void     on_read    (const StreamSP&, string&, const ErrorCode&);
    virtual void     on_write   (const StreamSP&, const ErrorCode&, const WriteRequestSP&);
    virtual void     on_eof     (const StreamSP&)
    // and some others ...
};

    MyListener lstn;
    tcp->event_listener(&lstn);
```
Be carefull with the lifetime. Handles expect a raw pointer and does not own listeners.

The parameters are the same as for callback version (first param is the event listener object itself, of course, second is the handle object, and other params are according to event documentation).


Event listener does not disable callbacks, i.e. if both event listener object and callbacks are set to a handle, both will be called (first callbacks, then event listener's method).

Event listener is convenient when some object makes use of several handles and wants to listen events from them. Then instead of setting many callbacks it can set itself as a listener for those handles. This saves cpu time and can make the code clearer.

```cpp
    struct MyClass : IStreamListener, ITimerListener {

    TimerSP timer;
    TcpSP stream;

    MyClass() {
        timer = new Timer();
        timer->start(10);
        timer->event_listener(this);

        tcp = new Tcp();
        tcp->connect(host, port);
        tcp->event_listener(this);
    }

    void on_timer(const TimerSP&) {}

    void on_connect(const StreamSP&, const ErrorCode&, const ConnectRequestSP&) {}
```

## Holding Handles

UniEvent does not hold a strong reference for created handle objects. You must hold them by yourself otherwise no events will be watched for.

```cpp
    Timer::create_once(1, [](auto){}); // timer is destroyed immediately as you didn't hold it
    Loop::default_loop->run(); // no events to watch, run() will return immediately
```

Correct way is


```cpp
    TimerSP timer = Timer::create_once(1, [](auto){}); // keep a strong reference
    Loop::default_loop->run();
```

Usually you have an object to place a refence to handle to. However sometimes it is convenient to capture handle in callback. Keep in mind that if you do this

```cpp
    TimerSP timer = new Timer(loop);
    timer->start(1, [timer](auto){
        //...
        if (smth) { timer->stop(); }
    });
```

yes, you will hold the timer, however you will create a cyclic reference timer -> callback -> timer and thus create a memory leak.
This code will stop the timer but will never destroy it. To remove cyclic reference, you need to break the cycle

```cpp
    TimerSP timer = new Timer(loop);
    timer->start(1, [timer](auto) mutable {
        //...
        if (smth) {
            timer->stop();
            timer.reset(); // reset iptr, not the timer
        }
    });
```

Notice that lambda has to be `mutable` for this.
Or you can remove callback from the timer handle.

```cpp
    TimerSP timer = new Timer(loop);
    timer->start(1, [timer](auto) {
        //...
        if (smth) {
            timer->stop();
            timer->event.remove_all();
        }
    });
```

## Async DNS

UniEvent supports for trully asyncronous resolve (async DNS). It does not use any threads and based on [c-ares](https://github.com/c-ares/c-ares).

You may use it indirectly, for example via tcp->connect(host, port) method or directly like this:

```cpp
    ResolverSP resolver = new Resolver(loop);
    resolver->resolve('myhost.com', [](const AddrInfo&, const std::error_code&, const RequestSP&) {
        // process result here
    });
    loop->run();
```

For more details, see [Resolver](doc/resolver.md), [Tcp](doc/tcp.md).

## Filesystem Async Operations

UniEvent provides a wide variety of cross-platform sync and async file system operations. For example:

```cpp
    Fs::stat(file, [](auto stat, auto err) {
        ...
    });

    Fs::mkstemp(template, [](auto path, auto fd, auto err) {
        ...
    });

    Loop::default_loop()->run();
```
See [Fs](doc/fs.md) for details.

## Utility Functions

UniEvent provides a number of cross-platform utility functions described in [function.md](doc/functions.md). Almost all of them are syncronous and not related to an event loop.

