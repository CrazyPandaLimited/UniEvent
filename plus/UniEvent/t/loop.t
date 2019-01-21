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
    ok !$loop->run_once, 'run_once';
    ok !$loop->run, 'run';
    alarm(0);
    ok !$loop->run_nowait, 'run_nowait';
};

subtest 'loop is alive while handle exists' => sub {
    my $h = new UniEvent::Prepare;
    $h->start(sub {
        ok($loop->alive, 'loop is alive while handle exists');
        $loop->stop;
    });
    alarm(1);
    ok $loop->run;
    alarm(0);
    pass 'loop stopped on stop';
    ok($loop->alive, 'loop is still alive');
    $h->stop;
    ok(!$loop->alive, 'handle stopped, loop is not alive');

    alarm(1);
    ok !$loop->run;
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
    ok !$loop->run;
    alarm(0);
    ok !$loop->alive, 'loop exits when no more active handles';
    
    $h->start;
    undef $h;
    alarm(1);
    ok !$loop->run;
    alarm(0);
    pass('loop exits when no more handles');
};

subtest 'handles' => sub {
    my $loop = new UniEvent::Loop;
    cmp_deeply $loop->handles, [], "no handles in fresh loop";
    
    my @h = map { new UniEvent::Prepare($loop) } 1..3;
    is scalar $loop->handles->@*, 3, "handles count ok";
    foreach my $h ($loop->handles->@*) {
        is ref($h), 'UniEvent::Prepare', 'handle class ok';
    }
    undef @h;
    
    cmp_deeply $loop->handles, [], "no handles left";
};

subtest 'non empty loop destroys all handles when destroyed' => sub {
    my $loop = new UniEvent::Loop;
    my $h = new UniEvent::Prepare($loop);
    $h->start(sub {});
    $loop->run_nowait;
    undef $loop;
    
    is $h->loop, undef, "handle has lost the loop";
    dies_ok { $h->stop } "handle can not be used without loop";
};

subtest 'call_soon' => sub {
    subtest 'simple' => sub {
        my $i = 0;
        $loop->call_soon(sub { $i++ });
        $loop->run_nowait for 1..3;
        is $i, 1, 'called once';
    };
    subtest 'recursive' => sub {
        my $i = 0;
        $loop->call_soon(sub { $i++ });
        $loop->run_nowait for 1..3;
        $loop->call_soon(sub {
            $i += 10;
            $loop->call_soon(sub { $i += 100 });
        });
        $loop->run_nowait for 1..3;
        is $i, 111, 'called';
    };
};

subtest 'CLONE_SKIP' => sub {
    is UniEvent::Loop::CLONE_SKIP(), 1;
};

subtest 'stop before run' => sub {
    my $loop = UniEvent::Loop->new();
    my $i;
    $loop->call_soon(sub { ++$i });
    $loop->stop;
    $loop->run_nowait;
    is $i, 1, "run works";
};

done_testing();
