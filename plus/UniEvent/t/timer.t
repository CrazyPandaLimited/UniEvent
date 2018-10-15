use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

catch_run('[timer]');

my ($l, $now, $t, $t2, $i, $j);

$l = UniEvent::Loop->default_loop;
$t = new UniEvent::Timer;

is($t->type, UniEvent::Handle::HTYPE_TIMER);

# once timer
$t->timer_callback(sub { $i++ });
time_mark();
$t->start(0, 0.02);
$l->run;
check_mark(0.02, "once: first call time is correct");
is($i, 1, "once: timer run once");
$i = 0;
time_mark();
$t->once(0.01);
$l->run;
check_mark(0.01, "once(v2): first call time is correct");
is($i, 1, "once(v2): timer run once");
$i = 0;

# call_now
$t->timer_callback(sub { $i++ });
$t->call_now for 1..6;
is($i, 6, "call_now works");

# stop
$t->timer_callback(sub { shift->stop });
$t->start(1, 0.01);
$l->run;
pass("stop works");

# initial = repeat
$t->timer_callback(sub {
    my $h = shift;
    $i++;
    check_mark(0.01, "initial=repeat: first call time and repeat call time are correct");
    time_mark();
    $h->stop if $i == 10;
});
time_mark();
$t->start(0.01);
$l->run;
is($i, 10, "initial=repeat: timer run 10 times");
$i = 0;

# different initial and repeat
$t->timer_callback(sub {
    my $h = shift;
    $i++;
    if ($i == 1) {
        check_mark(0.03, "initial!=repeat: first call time is corrent");
    } else {
        check_mark(0.01, "initial!=repeat: repeat call time is corrent");
    }
    time_mark();
    $h->stop if $i == 5;
});
time_mark();
$t->start(0.01, 0.03);
$l->run;
is($i, 5, "initial!=repeat: timer run 5 times");
$i = 0;


# change repeat
for my $initial_meth ('once', 'start') {
    $t = new UniEvent::Timer;
    $t->timer_callback(sub { $l->stop });
    $t->$initial_meth(0.01);
    $t->repeat(0.02);
    time_mark();
    $l->run;
    check_mark(0.01, "changing repeat doesn't apply for the next call ($initial_meth)");
    time_mark();
    $l->run;
    check_mark(0.02, "changing repeat applies for further calls ($initial_meth)");
}
$i = 0;

# again
$i = $j = 0;
$t  = new UniEvent::Timer;
$t2 = new UniEvent::Timer;
ok(!eval {$t->again; 1}, "again cannot be called on never-started timer");
$t->timer_callback(sub {
    if (++$i == 1) {
        check_mark(0.01, "again: first call in 0.01 because last time t2 didn't reset");
    } else {
        check_mark(0.02, "again: repeating ok after again");
    }
    shift->stop if $i > 5;
    time_mark();
});
$t2->timer_callback(sub { # $t2 holds $t from triggering by reseting it via again
    is($i, 0, "'again' resets repeating timer");
    if (++$j < 5) {
        $t->again;
    } else {
        $t2->stop;
        time_mark();
    }
});
$t->start(0.02);
$t2->start(0.01);
$l->run;
$i = $j = 0;

# reset
$t->timer_callback(sub { shift->reset });
time_mark();
$t->start(0.02);
$l->run;
check_mark(0.02, "reset works");

done_testing();
