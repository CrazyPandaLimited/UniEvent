use 5.012;
use warnings;
use lib 't'; use PETest;
use UniEvent::Error qw/ERRNO_EBUSY/;
use UniEvent::Handle;

my ($loop, $now, $h, $err, @hlist, $var);

$loop = UniEvent::Loop->new();
is(ref $loop, 'UniEvent::Loop');

ok(!$loop->alive, 'no handles yet in loop');
ok(!$loop->is_default, 'this loop is not default');

$loop->backend_timeout; # just check it works

$now = $loop->now;
select undef, undef, undef, 0.01;
is($now, $loop->now, 'now isnt changed until update_time');
$loop->update_time;
isnt($now, $loop->now, 'now changes after update_time');

$loop = UniEvent::Loop->default_loop;
is(ref $loop, 'UniEvent::Loop');
ok(!$loop->alive, 'no handles yet in loop');
ok($loop->is_default, 'this loop is default');

$now = $loop->now;
select undef, undef, undef, 0.01;
is($now, $loop->now);
UniEvent::Loop->default_loop->update_time;
isnt($now, $loop->now, 'default_loop always returns same objects');

alarm(1);
$loop->run_once;
$loop->run;
alarm(0);
is($loop->run_nowait, 0, "loop is empty so that doesnt block");

$now = $loop->now;
select undef, undef, undef, 0.01;
$loop->close;
$loop = UniEvent::Loop->default_loop;
isnt($now, $loop->now, 'default_loop is recreated after close (another object)');

$h = new UniEvent::Prepare;
$h->start(sub {
    ok($loop->alive, 'loop is alive while handle exists');
    $loop->stop;
});
alarm(1);
$loop->run;
alarm(0);
is(1, 1, 'loop stopped on stop');
ok($loop->alive, 'loop is still alive');
$h->stop;
ok(!$loop->alive, 'handle stopped, loop is not alive');

alarm(1);
$loop->run;
alarm(0);
ok(1, 'loop won\'t run anymore');

$h->start(sub {
    ok($loop->alive, 'loop is alive while handle exists 2');
    my $hh = shift;
    $hh->stop;
});
alarm(1);
$loop->run;
alarm(0);
ok(!$loop->alive, 'loop exits when no more active handles');

$h->start(sub {});

undef $h;
alarm(1);
$loop->run;
alarm(0);
is(1, 1, 'loop exits when no more handles');
$loop->close();
$loop = undef;

$loop = new UniEvent::Loop;
$h = new UniEvent::Prepare($loop);
$h->start(sub {});
$loop->run_nowait;
dies_ok { $loop->close } 'Non-empty loop dies on close';
$err = $@;
ok($err, 'error exists');
is(ref $err, 'UniEvent::CodeError', 'error is an object');
is($err->name, 'EBUSY', 'error is EBUSY');
is($err->code, ERRNO_EBUSY, 'error code is correct');

undef $h;
$loop->run_nowait;
ok(!$loop->alive, 'loop isnt alive anymore');
$loop->close();
$loop = undef;

$loop = UniEvent::Loop::default_loop();
@hlist = (new UniEvent::Prepare(), new UniEvent::Prepare(), new UniEvent::Prepare());
#map { $_->start(sub {}) } @hlist;
$var = 0;
$loop->walk(sub {
    my $cur_handle = shift;
    is(ref $cur_handle, 'UniEvent::Prepare', 'walk handle works');
    $var++;
});
is($var, 3, 'walk walked through all the handles');

# CLONE_SKIP
is(UniEvent::Loop::CLONE_SKIP() + UniEvent::Loop->CLONE_SKIP(), 2, "Loop has CLONE_SKIP set to 1");

done_testing();
