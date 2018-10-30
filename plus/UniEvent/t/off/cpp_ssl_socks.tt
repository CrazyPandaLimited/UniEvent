use 5.012;
use warnings;

use UniEvent;
use Benchmark qw/timethese/;
use CPP::catch;
use XS::Loader;
use IO::Select;
use IO::Socket::Socks;
use IO::Socket::Socks ':constants';
use IO::Socket::INET;

use lib 't/lib';
use SocksTest;
use SanityChecker;

XS::Loader::load_tests();

my $pid = SocksTest::socks_test();

$ENV{"UNIEVENT_TEST_SOCKS"} = 1; 
$ENV{"UNIEVENT_TEST_SSL"} = 1;

catch_run("[panda-event][socks][ssl]");

kill 'KILL', $pid;
