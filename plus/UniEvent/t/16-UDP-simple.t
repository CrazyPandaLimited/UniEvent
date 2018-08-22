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

my $initer = sub {
  my $udp = $_[0];
  my $sock;
  socket($sock, PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  $port = Binder::bind2free(Binder::make_perl_bound_socket(
    $sock, sub {
      $udp->open($_[0]);
    }));
  return $port;
};

$initer->($udp);

ok($port, "Bound to port");

my @inet_p = UniEvent::inet_stop($udp->getsockname());
is($inet_p[1], $port, "Port is real and coincides with got from getsockname()");

done_testing();
