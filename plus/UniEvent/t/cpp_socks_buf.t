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
$ENV{"UNIEVENT_TEST_BUF"} = 1; 

# test socks client with extra small buffer size
UniEvent::_run_cpp_tests_('[panda-event][socks]');

kill 'KILL', $pid;
