use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

my ($h, $err, @hlist, $var);

subtest 'basic' => sub {
    my $loop = UniEvent::Loop->new();
    is(ref $loop, 'UniEvent::Loop');
    ok(!$loop->alive, 'no handles yet in loop');
    ok(!$loop->is_default, 'this loop is not default');
    ok(!$loop->is_global, 'this loop is not global');
};

subtest 'now/update_time' => sub {
    my $loop = UniEvent::Loop->new();
    my $now = $loop->now;
    select undef, undef, undef, 0.01;
    is($now, $loop->now, 'now isnt changed until update_time');
    $loop->update_time;
    isnt($now, $loop->now, 'now changes after update_time');
};

subtest 'default loop' => sub {
    my $loop = UniEvent::Loop->default;
    is(ref $loop, 'UniEvent::Loop');
    ok(!$loop->alive, 'no handles yet in loop');
    ok($loop->is_default, 'this loop is default');
    ok($loop->is_global, 'this loop is global');

    my $now = $loop->now;
    select undef, undef, undef, 0.01;
    is($now, $loop->now);
    UniEvent::Loop->default->update_time;
    isnt($now, $loop->now, 'default_loop always returns same objects');
    is $loop->now, UniEvent::Loop->global->now, "global loop is the same as default loop in main thread";
};

my $loop = UniEvent::Loop->default;

subtest 'doesnt block when no handles' => sub {
    alarm(1);
    $loop->run_once;
    $loop->run;
    alarm(0);
    pass("doesnt block");
    is($loop->run_nowait, 0, "run nowait - no events");
};

subtest 'loop is alive while handle exists' => sub {
    my $h = new UniEvent::Prepare;
    $h->start(sub {
        ok($loop->alive, 'loop is alive while handle exists');
        $loop->stop;
    });
    alarm(1);
    $loop->run;
    alarm(0);
    pass 'loop stopped on stop';
    ok($loop->alive, 'loop is still alive');
    $h->stop;
    ok(!$loop->alive, 'handle stopped, loop is not alive');

    alarm(1);
    $loop->run;
    alarm(0);
    pass('loop won\'t run anymore');
};

subtest 'loop is alive while handle exists 2' => sub {
    my $h = new UniEvent::Prepare;
    $h->start(sub {
        my $hh = shift;
        ok $loop->alive, "loop is alive";
        $hh->stop;
    });
    alarm(1);
    $loop->run;
    alarm(0);
    ok !$loop->alive, 'loop exits when no more active handles';
    
    $h->start;
    undef $h;
    alarm(1);
    $loop->run;
    alarm(0);
    pass('loop exits when no more handles');
};

subtest 'handles' => sub {
    my $loop = new UniEvent::Loop;
    my $hl = $loop->handles;
    is @$hl, 0, "no handles in fresh loop";
    
    my $h = new UniEvent::Prepare($loop);
    $hl = $loop->handles;
    is @$hl, 1, "one handle";
};

#$loop->close();
#$loop = undef;
#
#$loop = new UniEvent::Loop;
#$h = new UniEvent::Prepare($loop);
#$h->start(sub {});
#$loop->run_nowait;
#dies_ok { $loop->close } 'Non-empty loop dies on close';
#$err = $@;
#ok($err, 'error exists');
#is(ref $err, 'UniEvent::CodeError', 'error is an object');
#is($err->name, 'EBUSY', 'error is EBUSY');
#is($err->code, ERRNO_EBUSY, 'error code is correct');
#
#undef $h;
#$loop->run_nowait;
#ok(!$loop->alive, 'loop isnt alive anymore');
#$loop->close();
#$loop = undef;
#
#$loop = UniEvent::Loop::default_loop();
#@hlist = (new UniEvent::Prepare(), new UniEvent::Prepare(), new UniEvent::Prepare());
##map { $_->start(sub {}) } @hlist;
#$var = 0;
#$loop->walk(sub {
#    my $cur_handle = shift;
#    is(ref $cur_handle, 'UniEvent::Prepare', 'walk handle works');
#    $var++;
#});
#is($var, 3, 'walk walked through all the handles');
#
## CLONE_SKIP
#is(UniEvent::Loop::CLONE_SKIP() + UniEvent::Loop->CLONE_SKIP(), 2, "Loop has CLONE_SKIP set to 1");

say "EPTA";

done_testing();
