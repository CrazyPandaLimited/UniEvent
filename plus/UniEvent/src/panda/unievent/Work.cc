#include <panda/unievent/Work.h>
#include <panda/unievent/global.h>
using namespace panda::unievent;

void Work::uvx_on_work (uv_work_t* req) {
    Work* r = rcast<Work*>(req);
    r->on_work();
}

void Work::uvx_on_after_work (uv_work_t* req, int) {
    Work* r = rcast<Work*>(req);
    r->on_after_work();
}

void Work::queue () {
    int err = uv_queue_work(uvr.loop, &uvr, uvx_on_work, uvx_on_after_work);
    if (err) throw CodeError(err);
}

void Work::on_work () {
    if (work_event) work_event(this);
    else throw ImplRequiredError("Work::on_work");
}

void Work::on_after_work () {
    if (after_work_event) after_work_event(this);
    else throw ImplRequiredError("Work::on_after_work");
}
