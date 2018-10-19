#include "Barrier.h"
using namespace panda::unievent;

int Barrier::wait () { return uv_barrier_wait(&handle); }

Barrier::~Barrier () {
    uv_barrier_destroy(&handle);
}
