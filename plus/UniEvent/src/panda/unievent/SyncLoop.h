#pragma once
#include "Loop.h"

namespace panda { namespace unievent {

struct SyncLoop {
    using Backend = backend::Backend;

    static const LoopSP& get (Backend* b) {
        for (const auto& row : loops) if (row.backend == b) return row.loop;
        loops.push_back({b, new Loop(b)});
        return loops.back().loop;
    }

private:
    struct Item {
        Backend* backend;
        LoopSP   loop;
    };
    static thread_local std::vector<Item> loops;
};

}}
