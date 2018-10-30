use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

my ($l, $p, $i, $j, $t);

$i = $j = 0;
$l = UniEvent::Loop->default_loop;
$p = new UniEvent::Prepare;
$t = new UniEvent::Timer;
$t->timer_callback(sub { $l->stop if $j++ > 10 });
$t->start(0.001);

is($p->type, UniEvent::Handle::HTYPE_PREPARE);

# start
$p->prepare_callback(sub { $i++ });
$p->start;
$l->run;
cmp_ok($i, '>', 0, "prepare works");
$i = $j = 0;

# stop
$p->stop;
$l->run;
is($i, 0, "stop works");
$i = $j = 0;

# reset
$p->start(sub { $i-- });
$l->run;
cmp_ok($i, '<', 0, "start sets prepare_callback");
$i = $j = 0;
$p->reset;
$l->run;
is($i, 0, "reset works");

my $called = 0;
UniEvent::Prepare::call_soon(sub {
    $called = 1;
});
$l->run;
ok($called, 'call_soon');

done_testing();
