#include "BackendLoop.h"
#include "BackendHandle.h"

using namespace panda::unievent::backend;

void BackendLoop::capture_exception () {
    _exceptions.push_back(std::current_exception());
    assert(_exceptions.back());
    stop();
}

void BackendLoop::throw_exceptions () {
    auto list = std::move(_exceptions);
    std::rethrow_exception(list[0]); // TODO: throw all exceptions as nested
}

void BackendHandle::capture_exception () {
    loop()->capture_exception();
}

void BackendRequest::capture_exception () {
    handle()->capture_exception();
}
