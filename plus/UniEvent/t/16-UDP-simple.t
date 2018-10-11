use strict;
use warnings;
use lib 't'; use PETest;
use UniEvent::Error;
use UniEvent::UDP;
use Test::More;
use Binder;
use Scalar::Util qw/weaken/;
use Socket qw(PF_INET IPPROTO_UDP SOMAXCONN SOCK_DGRAM inet_aton);
use Binder;
use IPC::Open2;

my $port;

my $udp = new UniEvent::UDP;

is($udp->type, UniEvent::Handle::HTYPE_UDP, "new udp object type");

$udp->bind("localhost", 0);
my ($host, $port) = UniEvent::inet_stop($udp->getsockname());

ok($port, "Bound to port");

done_testing();
