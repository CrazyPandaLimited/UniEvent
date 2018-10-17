use strict;
use warnings;
use lib 't/lib'; use MyTest;

my $udp = new UniEvent::UDP;
is($udp->type, UniEvent::Handle::HTYPE_UDP, "new udp object type");

$udp->bind("localhost", 0);
my ($host, $port) = UniEvent::inet_stop($udp->getsockname());

ok($port, "Bound to port");

done_testing();
