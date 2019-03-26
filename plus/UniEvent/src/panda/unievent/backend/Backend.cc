#include "BackendLoop.h"
#include "BackendHandle.h"

using namespace panda::unievent::backend;

void BackendLoop::capture_exception () {
    _exception = std::current_exception();
    assert(_exception);
    stop();
}

void BackendHandle::capture_exception () {
    loop()->capture_exception();
}

void BackendRequest::capture_exception () {
    handle()->capture_exception();
}
