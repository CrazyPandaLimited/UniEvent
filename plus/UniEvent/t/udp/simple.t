use strict;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;
BEGIN { plan skip_all => 'disabled'; }
my $udp = new UniEvent::UDP;
is($udp->type, UniEvent::Handle::HTYPE_UDP, "new udp object type");

$udp->bind(SA_LOOPBACK_ANY);
my $sa = $udp->get_sockaddr;

ok($sa->port, "Bound to port");

done_testing();
