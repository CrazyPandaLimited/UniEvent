#pragma once

struct EngineLoop {
    EngineLoop () = 0;

    EngineTimer* new_timer () = 0;

    virtual ~EngineLoop () {}
};
