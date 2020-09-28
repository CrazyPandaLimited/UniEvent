#include "streamer.h"
#include <panda/unievent/streamer/File.h>

#define TEST(name) TEST_CASE("streamer-file: " name, "[streamer-file]")

using namespace panda::unievent::streamer;

namespace {
    struct TestFileInput : FileInput {
        using FileInput::FileInput;

        int stop_reading_cnt = 0;

        void stop_reading () override {
            stop_reading_cnt++;
            FileInput::stop_reading();
        }
    };
}

TEST("normal") {
    AsyncTest test(30000, 1);
    auto i = new TestFileInput("t/streamer/file.txt", 10000);
    auto o = new TestOutput(20000);
    StreamerSP s = new Streamer(i, o, 100000, test.loop);
    s->start();
    s->finish_event.add([&](const ErrorCode& err) {
        if (err) WARN(err);
        CHECK(!err);
        test.happens();
    });
    test.run();
    CHECK(i->stop_reading_cnt == 0);
}

TEST("pause input") {
    AsyncTest test(30000, 1);
    auto i = new TestFileInput("t/streamer/file.txt", 30000);
    auto o = new TestOutput(10000);
    StreamerSP s = new Streamer(i, o, 50000, test.loop);
    s->start();
    s->finish_event.add([&](const ErrorCode& err) {
        if (err) WARN(err);
        CHECK(!err);
        test.happens();
    });
    test.run();
    CHECK(i->stop_reading_cnt > 0);
}
