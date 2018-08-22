#include <panda/unievent/Semaphore.h>
using namespace panda::unievent;

void Semaphore::post () {
    uv_sem_post(&handle);
}

void Semaphore::wait () {
    uv_sem_wait(&handle);
}

int Semaphore::trywait () {
    return uv_sem_trywait(&handle);
}

Semaphore::~Semaphore () {
    uv_sem_destroy(&handle);
}
