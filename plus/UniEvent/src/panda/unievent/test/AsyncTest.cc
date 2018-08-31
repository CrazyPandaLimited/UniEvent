#include "AsyncTest.h"
#include <sstream>
#include <iostream>

namespace panda { namespace unievent { namespace test {

AsyncTest::AsyncTest(uint64_t timeout, const std::vector<string>& expected)
    : loop(new Loop())
    , expected(expected)
    , timer(create_timeout(timeout))
    , broken_state(false)
{}

AsyncTest::~AsyncTest() noexcept(false) {
    timer.reset();
    loop->run_nowait(); // wait for all events trigered
    loop->run_nowait(); // in case of async close
    if (!broken_state && !happened_as_expected()) {
        throw Error("Test exits in bad state", *this);
    }

    get_thread_local_cached_resolver()->clear();
}

void AsyncTest::run() {
    try {
        loop->run();
    } catch (AsyncTest::Error& e) {
        //assume that test can not be continued after our own error
        string loop_err = destroy_loop();
        string err(e.what());
        if (loop_err) {
            err += ",\nAlso there was a problem with destroing a loop after this:\n" + loop_err;
        }
        throw Error(err, *this);
    } catch(panda::unievent::Error& e) {
        throw Error(e.what(), *this);
    } catch (std::exception& e) {
        throw Error(e.what(), *this);
    }
}

void AsyncTest::happens(string event) {
    if (event) {
        happened.push_back(event);
    }
}

std::string AsyncTest::generate_report() {
    std::stringstream out;

    for (size_t i = 0; i < std::max(expected.size(), happened.size()); ++i) {
        if (i >= expected.size()) {
            out << "\t\"" << happened[i] << "\" was not expected" << std::endl;
            continue;
        }
        if (i >= happened.size()) {
            out << "\t\"" << expected[i] << "\" has not happened" << std::endl;
            continue;
        }
        if (happened[i] != expected[i]) {
            out << "\t" << "wrong event " << happened[i] << ", " << expected[i] << " expected at pos " << i << std::endl;
            break;
        }
    }
    if (happened_as_expected()) {
        out << "OK" << std::endl;
    } else {
        out << "Expected: ";
        for (auto& e : expected) {
            out << "\"" << e << "\",";
        }
        out << std::endl << "Happened: ";
        for (auto& h : happened) {
            out << "\"" << h << "\",";
        }
        out << std::endl;
    }
    return out.str();
}

string AsyncTest::destroy_loop() {
    try {
        loop = nullptr;
    } catch (LoopError& e) {
        // we have a problem with loop, destructor will throw anyway and we do not want it
        // to prevent destructor from call
        loop->retain(); // yes it is memory leak, but test will fail anyway
        broken_state = true;
        return e.whats();
    }
    return "";
}

bool AsyncTest::happened_as_expected() {
    if (happened.size() != expected.size()) {
        return false;
    }
    for (size_t i = 0; i < happened.size(); ++i) {
        if (happened[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

sp<Timer> AsyncTest::create_timeout(uint64_t timeout) {
    return timer_once(timeout, loop, [&]() {
        throw Error("AsyncTest timeout", *this);
    });
}

AsyncTest::Error::Error(std::string msg, AsyncTest& test)
    : std::runtime_error(msg + "\nAsyncTest report:\n" + test.generate_report())
{}

}}}
