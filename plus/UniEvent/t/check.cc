#include "lib/test.h"

TEST_CASE("check", "[check]") {
    auto l = Loop::default_loop();
    AsyncTest test(100, {}, l);
    int cnt = 0;

    SECTION("start/stop/reset") {
        CheckSP h = new Check;
        CHECK(h->type() == Check::TYPE);

        h->check_event.add([&](const CheckSP&){ cnt++; });
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

    SECTION("runs after prepare") {
        PrepareSP p = new Prepare;
        p->start([&](const PrepareSP&){ cnt++; });
        CheckSP c = new Check;
        c->start([&](const CheckSP&) {
            CHECK(cnt == 1);
            cnt += 10;
        });
        l->run_nowait();
        CHECK(cnt == 11);
    }

    SECTION("call_now") {
        CheckSP h = new Check;
        h->check_event.add([&](const CheckSP&){ cnt++; });
        for (int i = 0; i < 5; ++i) h->call_now();
        CHECK(cnt == 5);
    };
}
