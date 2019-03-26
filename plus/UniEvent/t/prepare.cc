#include "lib/test.h"

TEST_CASE("prepare", "[prepare]") {
    auto l = Loop::default_loop();
    AsyncTest test(100, {}, l);
    int cnt = 0;

    SECTION("start/stop/reset") {
        PrepareSP h = new Prepare;
        CHECK(h->type() == Prepare::TYPE);

        h->event.add([&](const PrepareSP&){ cnt++; });
        h->start();
        CHECK(l->run_nowait());
        CHECK(cnt == 1);

        h->stop();
        CHECK(!l->run_nowait());
        CHECK(cnt == 1);

        h->start();
        CHECK(l->run_nowait());
        CHECK(cnt == 2);

        h->reset();
        CHECK(!l->run_nowait());
        CHECK(cnt == 2);
    }

    SECTION("call_now") {
        PrepareSP h = new Prepare;
        h->event.add([&](const PrepareSP&){ cnt++; });
        for (int i = 0; i < 5; ++i) h->call_now();
        CHECK(cnt == 5);
    };

    SECTION("exception safety") {
        PrepareSP h = new Prepare;
        h->event.add([&](const PrepareSP&){ cnt++; if (cnt == 1) throw 10; });
        h->start();
        try {
            l->run_nowait();
        }
        catch (int err) {
            CHECK(err == 10);
            cnt++;
        }
        CHECK(cnt == 2);

        l->run_nowait();
        CHECK(cnt == 3);
    }
}
