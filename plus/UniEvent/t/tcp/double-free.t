use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;

alarm(100);

my $loop = UniEvent::Loop->default_loop;
my $srv = UniEvent::TCP->new($loop);
$srv->bind("localhost", 0);
$srv->listen(128);

my $connected;
$srv->connection_callback(sub {
    my ($self, $cli, $err) = @_;
    fail $err if $err;
    $connected++;
});

my $counter = 0;

my $t = UniEvent::Prepare->new;
$t->prepare_callback(sub {
    my $cl = new UniEvent::TCP;
    $cl->connect($srv->get_sockaddr, sub {
        my ($handler, $err) = @_;
        fail $err if $err;
        if (++$counter == 1000) {
            $t->stop;
            $srv->reset;
            return;
        }
        $cl->write('GET /gcm/send HTTP/1.0\r\n\r\n', sub { $_[0]->disconnect; });
    });
});

$t->start;

$loop->run;

is($connected, 1000);

done_testing();
