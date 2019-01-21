use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

my $l = UniEvent::Loop->default_loop;

subtest 'start/stop/reset' => sub {
    my $h = new UniEvent::Prepare;
    is $h->type, UniEvent::Prepare::TYPE, 'type ok';
    
    my $i = 0;
    $h->prepare_callback(sub { $i++ });
    $h->start;
    ok $l->run_nowait, 'holds loop';
    is $i, 1, 'prepare works';
    
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

subtest 'call_now' => sub {
    my $h = new UniEvent::Prepare;
    my $i = 0;
    $h->prepare_callback(sub { $i++ });
    $h->call_now for 1..5;
    is $i, 5;
};

subtest 'zombie mode' => sub {
    my $l = new UniEvent::Loop;
    my $h = new UniEvent::Prepare($l);
    $h->prepare_callback(sub { fail("must not get called") });
    $h->start;
    undef $l;
    is $h->loop, undef, "loop";
    dies_ok { $h->start } "start";
    dies_ok { $h->stop  } "stop";
    dies_ok { $h->reset } "reset";
    undef $h;
};

done_testing();
