#include <panda/string.h>


using panda::string;

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

bool _run_cpp_tests_impl_(string arg) {
    signal(SIGPIPE, SIG_IGN);

    arg.append(1, '\0');
    std::vector<const char*> argv = {"test", "-i", arg.data()};
    return Catch::Session().run(argv.size(), argv.data()) == 0;
}

