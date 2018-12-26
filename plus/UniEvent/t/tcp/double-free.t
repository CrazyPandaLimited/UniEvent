use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;

alarm(10);

my $loop = UniEvent::Loop->default_loop;
my $srv = UniEvent::TCP->new($loop);
$srv->bind("localhost", 0);
$srv->listen(128);

my @d;

my $connected;
$srv->connection_callback(sub {
    my ($self, $cli, $err) = @_;
    fail $err if $err;
    $loop->stop if ++$d[2] == 1000;
});

my $t = UniEvent::Prepare->new;
$t->prepare_callback(sub {
    my $cl = new UniEvent::TCP;
    $cl->connect($srv->get_sockaddr, sub {
        my ($handler, $err) = @_;
        fail $err if $err;
        return if ++$d[1] == 1000;
        $cl->write('GET /gcm/send HTTP/1.0\r\n\r\n', sub { $_[0]->disconnect; });
    });
    $t->stop if ++$d[0] == 1000;
});

$t->start;

$loop->run;

cmp_deeply(\@d, [1000, 1000, 1000]);

done_testing();
