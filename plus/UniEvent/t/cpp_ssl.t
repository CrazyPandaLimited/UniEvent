use 5.012;
use warnings;
use UniEvent;

use lib 't/lib';
use SanityChecker;

$ENV{"UNIEVENT_TEST_SSL"} = 1;
UniEvent::_run_cpp_tests_('[panda-event][ssl]');
