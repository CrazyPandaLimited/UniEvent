#include <panda/unievent/RWLock.h>
using namespace panda::unievent;

void RWLock::rdlock () {
    uv_rwlock_rdlock(&handle);
}

void RWLock::wrlock () {
    uv_rwlock_wrlock(&handle);
}

void RWLock::rdunlock () {
    uv_rwlock_rdunlock(&handle);
}

void RWLock::wrunlock () {
    uv_rwlock_wrunlock(&handle);
}

int RWLock::tryrdlock () {
    return uv_rwlock_tryrdlock(&handle);
}

int RWLock::trywrlock () {
    return uv_rwlock_trywrlock(&handle);
}

RWLock::~RWLock () {
    uv_rwlock_destroy(&handle);
}
