#pragma once
#include "Loop.h"

namespace panda { namespace unievent {

struct SyncLoop {
    using Backend = backend::Backend;

    static const LoopSP& get (Backend* b) {
        auto& list = loops;
        for (const auto& row : list) if (row.backend == b) return row.loop;
        list.push_back({b, new Loop(b)});
        return list.back().loop;
    }

private:
    struct Item {
        Backend* backend;
        LoopSP   loop;
    };
    static thread_local std::vector<Item> loops;
};

}}
