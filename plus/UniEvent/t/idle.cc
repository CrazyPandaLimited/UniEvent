#include "lib/test.h"

TEST_CASE("idle", "[idle]") {
    auto l = Loop::default_loop();
    AsyncTest test(1000, {}, l);
    int cnt = 0;

    SECTION("start/stop/reset") {
        IdleSP h = new Idle;
        CHECK(h->type() == Idle::TYPE);

        h->idle_event.add([&](const IdleSP&){ cnt++; });
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

    SECTION("runs rarely when loop is high loaded") {
        TimerSP t = new Timer;
        t->timer_event.add([](const TimerSP& t){
            static int j = 0;
            if (++j % 10 == 0) t->loop()->stop();
        });
        t->start(1);

        int cnt = 0;
        IdleSP h = new Idle;
        h->start([&](const IdleSP&) { cnt++; });
        l->run();

        int low_loaded_cnt = cnt;
        cnt = 0;

        std::vector<TimerSP> v;
        while (v.size() < 10000) {
            v.push_back(new Timer);
            v.back()->timer_event.add([](const TimerSP&){});
            v.back()->start(1);
        }

        l->run();
        CHECK(cnt < low_loaded_cnt); // runs rarely

        int high_loaded_cnt = cnt;
        cnt = 0;
        v.clear();
        l->run();
        CHECK(cnt > high_loaded_cnt); // runs often again
    }

    SECTION("call_now") {
        IdleSP h = new Idle;
        h->idle_event.add([&](const IdleSP&){ cnt++; });
        for (int i = 0; i < 5; ++i) h->call_now();
        CHECK(cnt == 5);
    }
}
