use 5.020;
use warnings;

use Test::More;
use UniEvent;
use Socket;

plan skip_all => 'set WITH_LEAKS=1 to enable leaks test' unless $ENV{WITH_LEAKS};

my $loop = UniEvent::Loop->default_loop;

my $counter = 0;

my $t = UniEvent::Timer->new;
$t->timer_callback( sub {

my $cl = new UniEvent::TCP();
$cl->connect('google.com', 80, 0, undef, sub {
    my ($handler, $err) = @_;
    die $err if $err;

    if(++ $counter == 1000) {
        $t->stop();
        return;
    }

    $cl->write('GET /gcm/send HTTP/1.0\r\n\r\n', sub { $_[0]->disconnect; });
});

});

$t->start(0.01);

$loop->run();

ok(1);

done_testing();
