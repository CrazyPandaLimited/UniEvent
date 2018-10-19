use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;

my $loop = UniEvent::Loop->default_loop;

my $s = new UniEvent::TCP;
$s->bind(SA_LOOPBACK_ANY);
$s->listen;
$s->connection_callback(sub {});
my $sa = $s->get_sockaddr;

my $cl = new UniEvent::TCP;
$cl->connect($sa, sub {
    my ($handler, $err) = @_;
    fail $err if $err;
    pass("first connected");
});
$cl->write('1');
$cl->disconnect;
$cl->connect($sa, sub {
    my ($handler, $err) = @_;
    fail $err if $err;
    pass("second connected");
	$loop->stop;
});

$loop->update_time;
$loop->run;

done_testing(2);
