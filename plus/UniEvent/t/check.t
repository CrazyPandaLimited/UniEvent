use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

my $l = UniEvent::Loop->default_loop;

subtest 'start/stop/reset' => sub {
    my $h = new UniEvent::Check;
    is $h->type, UniEvent::Check::TYPE, 'type ok';
    
    my $i = 0;
    $h->check_callback(sub { $i++ });
    $h->start;
    ok $l->run_nowait, 'holds loop';
    is $i, 1, 'check works';
    
    $h->stop;
    ok !$l->run_nowait, 'stopped';
    is $i, 1, 'stop works';
    
    $h->start;
    ok $l->run_nowait;
    is $i, 2, 'started again';

    $h->reset;
    ok !$l->run_nowait;
    is $i, 2, 'reset works';
};

subtest 'runs after prepare' => sub {
    my $i = 0;
    my $p = new UniEvent::Prepare;
    $p->start(sub { $i++ });
    my $c = new UniEvent::Check;
    $c->start(sub {
        is $i, 1, 'after prepare';
        $i += 10;
    });
    $l->run_nowait;
    is $i, 11, 'called';
};

subtest 'zombie mode' => sub {
    my $l = new UniEvent::Loop;
    my $h = new UniEvent::Check($l);
    $h->check_callback(sub { fail("must not get called") });
    $h->start;
    undef $l;
    is $h->loop, undef, "loop";
    dies_ok { $h->start } "start";
    dies_ok { $h->stop  } "stop";
    dies_ok { $h->reset } "reset";
    undef $h;
};

done_testing();
