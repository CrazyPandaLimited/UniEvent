use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

catch_run('[timer]');

my $l = UniEvent::Loop->default;

subtest 'once timer' => sub {
    $l->update_time;
    my $t = new UniEvent::Timer;
    
    is $t->type, UniEvent::Timer::TYPE, 'type correct';
    
    $t->timer_callback(\&count);
    time_mark();
    $t->start(0, 0.02);
    $l->run;
    check_mark(0.02, "first call time is correct");
    check_count(1, "timer run once");
    
    time_mark();
    $t->once(0.01);
    $l->run;
    check_mark(0.01, "first call time is correct");
    check_count(1, "timer run once");
    
    ok !$l->alive, "loop is not alive after firing";
    $l->run;
    pass "loop doesnt get blocked after firing";
};

subtest 'call_now' => sub {
    my $t = new UniEvent::Timer;
    $t->timer_callback(\&count);
    $t->call_now for 1..6;
    check_count(6, "call_now works");
};

subtest 'stop' => sub {
    my $t = new UniEvent::Timer;
    $t->timer_callback(sub { shift->stop });
    $t->start(1, 0.01);
    $l->run;
    pass "stop works";
};

subtest 'initial = repeat' => sub {
    $l->update_time;
    my $t = new UniEvent::Timer;
    $t->timer_callback(sub {
        my $h = shift;
        count();
        check_mark(0.01, "first call time and repeat call time are correct");
        time_mark();
        $h->stop if ++(state $i) == 10;
    });
    time_mark();
    $t->start(0.01);
    $l->run;
    check_count(10, "timer run 10 times");
};

subtest 'different initial and repeat' => sub {
    $l->update_time;
    my $t = new UniEvent::Timer;
    $t->timer_callback(sub {
        my $h = shift;
        ++(state $i);
        count();
        $i == 1 ? check_mark(0.03, "first call time is corrent") :
                  check_mark(0.01, "repeat call time is corrent");
        time_mark();
        $h->stop if $i == 5;
    });
    time_mark();
    $t->start(0.01, 0.03);
    $l->run;
    check_count(5, "timer run 5 times");
};

subtest 'change repeat' => sub {
    my $sub = sub {
        $l->update_time;
        my $initial_meth = shift;
        my $t = new UniEvent::Timer;
        $t->timer_callback(sub { $l->stop });
        $t->$initial_meth(0.01);
        $t->repeat(0.02);
        time_mark();
        $l->run;
        check_mark(0.01, "changing repeat doesn't apply for the next call ($initial_meth)");
        time_mark();
        $l->run;
        check_mark(0.02, "changing repeat applies for further calls ($initial_meth)");
    };
    subtest 'once'  => $sub, 'once';
    subtest 'start' => $sub, 'start';
};

subtest 'again' => sub {
    $l->update_time;
    my $t  = new UniEvent::Timer;
    my $t2 = new UniEvent::Timer;
    dies_ok {$t->again} "again cannot be called on never-started timer";
    $t->timer_callback(sub {
        ++(state $i);
        count();
        $i == 1 ? check_mark(0.01, "first call in 0.01 because last time t2 didn't reset") :
                  check_mark(0.02, "repeating ok after again");
        shift->stop if $i > 5;
        time_mark();
    });
    $t2->timer_callback(sub { # $t2 holds $t from triggering by reseting it via again
        my $me = shift;
        check_count(0, "'again' resets repeating timer");
        ++(state $i);
        if ($i < 5) {
            $t->again;
        } else {
            $me->stop;
            time_mark();
        }
    });
    $t->start(0.02);
    $t2->start(0.01);
    $l->run;
    check_count(6, "first timer fired after second stops");
};

subtest 'reset' => sub {
    my $t = new UniEvent::Timer;
    $t->timer_callback(sub { shift->reset });
    time_mark();
    $t->start(0.02);
    $l->run;
    check_mark(0.02, "reset works");
};

subtest 'zombie mode' => sub {
    my $l = new UniEvent::Loop;
    my $t = new UniEvent::Timer($l);
    $t->timer_callback(sub { fail("must not get called") });
    $t->start(0.01);
    undef $l;
    is $t->loop, undef, "loop";
    dies_ok { $t->start(0.01)  } "start";
    dies_ok { $t->stop         } "stop";
    dies_ok { $t->reset        } "reset";
    dies_ok { $t->again        } "again";
    dies_ok { $t->repeat(0.01) } "repeat";
    dies_ok { $t->once(0.01)   } "once";
    undef $t;
};

done_testing();

my $cnt;

sub count { ++$cnt; }

sub check_count {
    is $cnt, shift(), shift();
    $cnt = 0;
}