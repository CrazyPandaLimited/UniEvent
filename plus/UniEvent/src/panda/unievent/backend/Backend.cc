#include "LoopImpl.h"
#include "HandleImpl.h"

using namespace panda::unievent::backend;

uint64_t HandleImpl::last_id;

void LoopImpl::capture_exception () {
    _exception = std::current_exception();
    assert(_exception);
    stop();
}

void LoopImpl::throw_exception () {
    auto exc = std::move(_exception);
    std::rethrow_exception(exc);
}
