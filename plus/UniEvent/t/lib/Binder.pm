package Binder;
use 5.012;
use warnings;
use UniEvent::Error;
use Scalar::Util 'blessed';
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
    } else {
        my $err = $@;
        die $err unless blessed($err) and $err->isa("UniEvent::Error");
        die $err if $err->code != ERRNO_EADDRINUSE and $err->code != ERRNO_EACCES;
    }
  }
  return $port;
}

sub make_perl_bound_socket{
  my ($sock, $init) = @_ ;
  return sub {
    die UniEvent::CodeError->new(ERRNO_EADDRINUSE) unless bind($sock, sa($_[0]));
    $init->($sock);
  };
}

1;
