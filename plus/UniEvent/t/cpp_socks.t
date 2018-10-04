use 5.012;
use warnings;

use UniEvent;
use Benchmark qw/timethese/;
use IO::Select;
use IO::Socket::Socks;
use IO::Socket::Socks ':constants';
use IO::Socket::INET;

use lib 't/lib';
use SanityChecker;
use SocksTest;

my $pid = SocksTest::socks_test();

$ENV{"UNIEVENT_TEST_SOCKS"} = 1; 

UniEvent::_run_cpp_tests_('[panda-event][socks]');

kill 'KILL', $pid;
