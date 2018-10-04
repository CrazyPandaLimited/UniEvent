use 5.012;
use warnings;
use UniEvent;

use lib 't/lib';
use SanityChecker;

UniEvent::_run_cpp_tests_($ENV{CPP_TEST} || '[panda-event]');
