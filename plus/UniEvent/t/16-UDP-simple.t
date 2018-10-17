use strict;
use warnings;
use lib 't/lib'; use MyTest;

my $udp = new UniEvent::UDP;
is($udp->type, UniEvent::Handle::HTYPE_UDP, "new udp object type");

$udp->bind("localhost", 0);
my $sa = $udp->get_sockaddr;

ok($sa->port, "Bound to port");

done_testing();
