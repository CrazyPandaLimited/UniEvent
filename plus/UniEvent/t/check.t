use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

my ($l, $c, $p, $i, $j, $x, $t);

$i = $j = 0;
$l = UniEvent::Loop->default_loop;
$c = new UniEvent::Check;
$p = new UniEvent::Prepare;
$t = new UniEvent::Timer;
$t->timer_callback(sub { $l->stop if $j++ > 10 });
$t->start(0.001);

is($c->type, UniEvent::Handle::HTYPE_CHECK);

# start
$p->prepare_callback(sub { $x++ });
$p->start;
$c->check_callback(sub { cmp_ok($x, '>', $i, "check is always after prepare"); $i++ });
$c->start;
$l->run;
cmp_ok($i, '>', 0, "check works");
$p->stop;
$i = $j = 0;

# stop
$c->stop;
$l->run;
is($i, 0, "stop works");
$i = $j = 0;

# reset
$c->start(sub { $i-- });
$l->run;
cmp_ok($i, '<', 0, "start sets check_callback");
$i = $j = 0;
$c->reset;
$l->run;
is($i, 0, "reset works");

done_testing();
