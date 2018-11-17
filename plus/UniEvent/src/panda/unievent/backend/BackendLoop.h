namespace panda { namespace unievent { namespace engine {

struct BackendLoop {
    virtual int  run         () = 0;
    virtual int  run_once    () = 0;
    virtual int  run_nowait  () = 0;
    virtual void stop        () = 0;
    virtual void handle_fork () = 0;

    virtual ~BackendLoop () {}
};

}}}
