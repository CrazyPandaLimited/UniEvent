use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;

alarm(100);

plan skip_all => 'set WITH_LEAKS=1 to enable' unless $ENV{WITH_LEAKS};

my $loop = UniEvent::Loop->default_loop;

my $counter = 0;

my $t = UniEvent::Timer->new;
$t->timer_callback(sub {
    my $cl = new UniEvent::TCP;
    $cl->connect('google.com', 80, 0, undef, sub {
        my ($handler, $err) = @_;
        fail $err if $err;
        return $t->stop if ++$counter == 1000;
        $cl->write('GET /gcm/send HTTP/1.0\r\n\r\n', sub { $_[0]->disconnect; });
    });
});

$t->start(0.01);

$loop->run;

pass();

done_testing();
