use 5.020;
use warnings;

use Test::More;
use UniEvent;
use Socket;

my $hints = UniEvent::addrinfo_hints(SOCK_STREAM, PF_INET);
my $loop = UniEvent::Loop->default_loop;
my $s = new UniEvent::TCP();
$s->bind('localhost',0, $hints);
$s->listen();
$s->connection_callback(sub {});
my $adr = $s->getsockname;
my ($port, $adrrrr) = sockaddr_in ($adr);
my $cl = new UniEvent::TCP();
$cl->connect('localhost', $port, 1, $hints, sub {
    my ($handler, $err) = @_;
    ok !$err;
    fail $err if $err;
    note "first connected";
});
$cl->write('1');
$cl->disconnect();
$cl->connect('localhost', $port, 1, $hints, sub {
    my ($handler, $err) = @_;
    ok !$err;
    fail $err if $err;
    note "second connected";
	$loop->stop();
});
$loop->update_time();
$loop->run();

done_testing();
