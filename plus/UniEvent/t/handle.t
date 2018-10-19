use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Handle;

my ($l, $t, $i);

# check constants existance
cmp_ok(
    HTYPE_UNKNOWN_HANDLE + HTYPE_PREPARE + HTYPE_PROCESS + HTYPE_FS_EVENT + HTYPE_FS_POLL + HTYPE_SIGNAL + HTYPE_NAMED_PIPE +
    HTYPE_POLL + HTYPE_ASYNC + HTYPE_CHECK + HTYPE_TCP + HTYPE_TIMER + HTYPE_TTY + HTYPE_UDP + HTYPE_IDLE + HTYPE_FILE + HTYPE_MAX,
    '>', 0, "handle constants exists"
);
is(UniEvent::Handle::CLONE_SKIP() + UniEvent::Handle->CLONE_SKIP(), 2, "Handle has CLONE_SKIP set to 1");

# check 'loop' method
$l = new UniEvent::Loop();
$t = new UniEvent::Timer($l);
is($t->loop, $l, "handle belongs to another loop and refs are the same as XSBackref is in effect");
undef $t; $l->run; undef $l; # wait for handle's close-event to be removed from loop
$l = UniEvent::Loop->default_loop;
$t = new UniEvent::Timer;
$t->timer_callback(sub {shift->stop});
ok($t->loop->is_default, "handle belongs to default loop");

# with_callbacks
ok($t->with_callbacks, "from-perl created objects are with callbacks");

# type
is($t->type, HTYPE_TIMER, "type works");

# active
ok(!$t->active, "not started handle is non-active");
$t->start(0.01);
ok($t->active, "started handle is active");
$l->run; # timer stops himself on callback
ok(!$t->active, "stopped handle is not active");

# weak
ok(!$t->weak, "by default, handle is non-weak");
time_mark();
$t->start(0.02);
$l->run;
check_mark(0.02, "non-weak handle doesn't allow loop to bail out");
$t->weak(1);
ok($t->weak, "handle is now weak");
time_mark();
$t->start(0.1);
$l->run;
check_mark(0, "loop without any non-weak handles, bails out immediately");

done_testing();
