#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_DEFAULT_REPORTER "tap"
#include <catch.hpp>
#include <catch_reporter_tap.hpp>

#include <panda/string.h>
#include <panda/unievent/Debug.h>

bool TEST_SSL;
bool TEST_SOCKS;
bool TEST_BUF;

bool _run_cpp_tests_impl_(panda::string arg) {
    signal(SIGPIPE, SIG_IGN);
    
    TEST_SSL = (bool)std::getenv("UNIEVENT_TEST_SSL");
    TEST_SOCKS = (bool)std::getenv("UNIEVENT_TEST_SOCKS");
    TEST_BUF = (bool)std::getenv("UNIEVENT_TEST_BUF");

    arg.append(1, '\0');

    // test - executable name
    // arg - test tag
    // s - show successes
    // i - show invisibles
    std::vector<const char*> argv = {"test", arg.data(), "-s", "-i"};
    Catch::Session session;    
    return session.applyCommandLine(argv.size(), argv.data()) == 0 && session.run() == 0;
}
