use strict;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;

my $udp = new UniEvent::Udp;
is($udp->type, UniEvent::Udp::TYPE, "new udp object type");

eval { $udp->bind(SA_LOOPBACK_ANY); };
warn $@;
#my $sa = $udp->get_sockaddr;
#
#ok($sa->port, "Bound to port");

done_testing();
