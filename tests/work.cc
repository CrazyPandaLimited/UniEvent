#include "lib/test.h"

TEST_PREFIX("work: ", "[work]");

TEST("main") {
    AsyncTest test(10000, {"w", "aw"});
    WorkSP w = new Work(test.loop);
    auto main_id = std::this_thread::get_id();
    w->work_cb = [&](Work*) {
        CHECK(std::this_thread::get_id() != main_id);
        test.happens("w");
    };
    w->after_work_cb = [&](auto, auto& err) {
        CHECK(!err);
        CHECK(std::this_thread::get_id() == main_id);
        test.happens("aw");
    };
    w->queue();
    test.run();
}

TEST("cancel") {
    AsyncTest test(10000);
    WorkSP w = new Work(test.loop);
    w->work_cb       = [&](Work*) { FAIL(); };
    w->after_work_cb = [&](auto...) { FAIL(); };
    w->cancel();
    test.run(); // noop
    // can't test active work because it starts executing immediately if there are free workers and thus
    // there is probability that it can't be canceled
}

TEST("factory") {
    AsyncTest test(10000, 2);
    auto r = Work::queue(
        [&](Work*) { test.happens(); },
        [&](const WorkSP&, const std::error_code&) { test.happens(); },
        test.loop
    );
    CHECK(r);
    r = nullptr; // request must be alive while not completed
    test.run();
}

TEST("event listener") {
    AsyncTest test(10000);
    WorkSP w = new Work(test.loop);
    auto s = [&](auto lst) {
        w->event_listener(&lst);

        w->queue();
        test.run();

        CHECK(lst.cnt == 11);
    };
    SECTION("std") {
        struct Lst : IWorkListener {
            int cnt = 0;
            void on_work       (Work*)                                 override { ++cnt; }
            void on_after_work (const WorkSP&, const std::error_code&) override { cnt += 10; }
        };
        s(Lst());
    }
    SECTION("self") {
        struct Lst : IWorkSelfListener {
            int cnt = 0;
            void on_work       ()                       override { ++cnt; }
            void on_after_work (const std::error_code&) override { cnt += 10; }
        };
        s(Lst());
    }
}
