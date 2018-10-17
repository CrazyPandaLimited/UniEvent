use 5.020;
use warnings;

use Test::More;
use UniEvent;
use Socket;

my $loop = UniEvent::Loop->default_loop;
my $s = new UniEvent::TCP();
$s->bind('localhost', 0);
$s->listen();
$s->connection_callback(sub {});
my $sa = $s->get_sockaddr;
my $cl = new UniEvent::TCP();
$cl->connect($sa, sub {
    my ($handler, $err) = @_;
    ok !$err;
    fail $err if $err;
    note "first connected";
});
$cl->write('1');
$cl->disconnect();
$cl->connect($sa, sub {
    my ($handler, $err) = @_;
    ok !$err;
    fail $err if $err;
    note "second connected";
	$loop->stop();
});
$loop->update_time();
$loop->run();

print $cl->dump() . "\n";

done_testing();
