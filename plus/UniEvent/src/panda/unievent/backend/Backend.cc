#include "BackendLoop.h"
#include "BackendHandle.h"

using namespace panda::unievent::backend;

uint64_t BackendHandle::last_id;

void BackendLoop::capture_exception () {
    _exception = std::current_exception();
    assert(_exception);
    stop();
}

void BackendLoop::throw_exception () {
    auto exc = std::move(_exception);
    std::rethrow_exception(exc);
}

void BackendHandle::capture_exception () {
    loop()->capture_exception();
}

void BackendRequest::capture_exception () {
    handle()->capture_exception();
}
