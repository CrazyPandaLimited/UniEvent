use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

my $l = UniEvent::Loop->default_loop;
my $t = new UniEvent::Timer;
$t->timer_callback(sub { $l->stop if ++(state $j) % 3 == 0 });
$t->start(0.001);

my $p = new UniEvent::Prepare;
is $p->type, UniEvent::Prepare::TYPE, 'type ok';

subtest 'start' => sub {
    my $i = 0;
    $p->prepare_callback(sub { $i++ });
    $p->start;
    $l->run;
    cmp_ok $i, '>', 0, "prepare works";
};

subtest 'stop' => sub {
    my $i = 0;
    $p->prepare_callback(sub { $i++ });
    $p->stop;
    $l->run;
    is $i, 0, "stop works";
};

subtest 'reset' => sub {
    my $i = 0;
    $p->prepare_callback(sub { $i++ });
    $p->reset;
    $l->run;
    is $i, 0, "reset works";
};

subtest 'prepare holds loop' => sub {
    my $l = new UniEvent::Loop;
    my $p = new UniEvent::Prepare($l);
    $p->start(sub {});
    ok $l->run_nowait;
    $p->stop;
    ok !$l->run_nowait;
};

subtest 'zombie mode' => sub {
    my $l = new UniEvent::Loop;
    my $p = new UniEvent::Prepare($l);
    $p->prepare_callback(sub { fail("must not get called") });
    $p->start;
    undef $l;
    is $p->loop, undef, "loop";
    dies_ok { $p->start } "start";
    dies_ok { $p->stop  } "stop";
    dies_ok { $p->reset } "reset";
    undef $p;
};

done_testing();
