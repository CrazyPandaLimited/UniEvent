#include "lib/test.h"

TEST_CASE("work", "[work]") {
    AsyncTest test(1000);
    WorkSP w = new Work(test.loop);

    SECTION("main") {
        test.set_expected({"w", "aw"});
        auto main_id = std::this_thread::get_id();
        w->work_cb = [&](const WorkSP&) {
            CHECK(std::this_thread::get_id() != main_id);
            test.happens("w");
        };
        w->after_work_cb = [&](const WorkSP&, const CodeError& err) {
            CHECK(!err);
            CHECK(std::this_thread::get_id() == main_id);
            test.happens("aw");
        };
        w->queue();
        test.run();
    }

    SECTION("cancel") {
        SECTION("not active") {
            w->work_cb       = [&](const WorkSP&) { FAIL(); };
            w->after_work_cb = [&](const WorkSP&, const CodeError&) { FAIL(); };
            w->cancel();
            test.run(); // noop
        }
        SECTION("active") {
            test.set_expected(1);
            w->work_cb       = [&](const WorkSP&) {};
            w->after_work_cb = [&](const WorkSP&, const CodeError& err) {
                CHECK(err.code() == std::errc::operation_canceled);
                test.happens();
            };
            w->queue();
            w->cancel();
            test.run(); // noop
        }
    }

    SECTION("factory") {
        test.set_expected(2);
        auto r = Work::queue(
            [&](const WorkSP&) { test.happens(); },
            [&](const WorkSP&, const CodeError&) { test.happens(); },
            test.loop
        );
        CHECK(r);
        r = nullptr; // request must be alive while not completed
        test.run();
    }
}
