use 5.012;
use warnings;
use lib 't'; use PETest;

my ($l, $idle, $i, $j, $t, $low_loaded_cnt, $high_loaded_cnt);

$i = $j = 0;
$l = UniEvent::Loop->default_loop;
$idle = new UniEvent::Idle;
$t = new UniEvent::Timer;
$t->timer_callback(sub { $l->stop if $j++ > 10 });
$t->start(0.001);

is($idle->type, UniEvent::Handle::HTYPE_IDLE);

# start
$idle->idle_callback(sub { $i++; });
$idle->start;
$l->run;
cmp_ok($i, '>', 0, "idle works");
$low_loaded_cnt = $i;
$i = $j = 0;

# stop
$idle->stop;
$l->run;
is($i, 0, "stop works");
$i = $j = 0;

# reset
$idle->start(sub { $i-- });
$l->run;
cmp_ok($i, '<', 0, "start sets idle_callback");
$i = $j = 0;
$idle->reset;
$l->run;
is($i, 0, "reset works");
$i = $j = 0;

# idle has less change to run when high loaded
my @a;
for (0..10000) {
    my $tt = new UniEvent::Timer;
    push @a, $tt;
    $tt->timer_callback(sub {});
    $tt->start(0.001);
}
$idle->idle_callback(sub { $i++; });
$idle->start;
$l->run;
cmp_ok($i, '<', $low_loaded_cnt, "idle runs rarely when the loop is high loaded");
$high_loaded_cnt = $i;
$i = $j = 0;
undef @a; $l->run;
cmp_ok($i, '>', $high_loaded_cnt, "idle runs often again");

done_testing();
