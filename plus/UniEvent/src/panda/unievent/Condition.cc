#include "Condition.h"
using namespace panda::unievent;

void Condition::signal () {
    uv_cond_signal(&handle);
}

void Condition::broadcast () {
    uv_cond_broadcast(&handle);
}

void Condition::wait (Mutex* mutex) {
    uv_cond_wait(&handle, _pex_(mutex));
}

int Condition::timedwait (Mutex* mutex, uint64_t timeout) {
    return uv_cond_timedwait(&handle, _pex_(mutex), timeout);
}

Condition::~Condition () {
    uv_cond_destroy(&handle);
}
