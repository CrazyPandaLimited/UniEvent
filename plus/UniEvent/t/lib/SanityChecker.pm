package SanityChecker;

my $full_tests = UniEvent->can('_run_cpp_tests_');
unless ($full_tests) {
    require Test::More;
    Test::More->import('no_plan');
    warn "rebuild Makefile.PL adding TEST_FULL=1 to enable all tests'";
    ok(1);
    exit(0);
}

unless ($ENV{UNIEVENT_FREE_PORT}) {
    require Test::More;
    Test::More->import;
    BAIL_OUT('C++ tests require UNIEVENT_FREE_PORT to be set');
}

1;
