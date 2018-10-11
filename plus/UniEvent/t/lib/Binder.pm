use strict;
use warnings;

package Binder;

use Socket qw(INADDR_ANY sockaddr_in);

sub sa {
  return scalar sockaddr_in($_[0], INADDR_ANY);
}

sub bind2free {
  my ($bind_and_init) = @_;
  my $port;
  for (49152..65535) {
    $port = $_;
    if (eval { $bind_and_init->($port); 1 }) {
      last;
    }
  }
  return $port;
}

sub make_perl_bound_socket{
  my ($sock, $init) = @_ ;
  return sub {
    die $! unless bind($sock, sa($_[0]));
    $init->($sock);
  };
}

1;
