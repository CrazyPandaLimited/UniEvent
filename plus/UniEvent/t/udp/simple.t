use strict;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;

my $udp = new UniEvent::Udp;
is($udp->type, UniEvent::Udp::TYPE, "new udp object type");

$udp->bind_sa(SA_LOOPBACK_ANY);
my $sa = $udp->sockaddr;

ok($sa->port, "Bound to port");

done_testing();
