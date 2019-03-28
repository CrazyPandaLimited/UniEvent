use 5.012;
use lib 't/lib';
use MyTest;
use UniEvent::Error;

catch_run('[resolver]');

my $l = UniEvent::Loop->default_loop();

subtest 'not cached' => \&test_resolve, 0;
subtest 'cached'     => \&test_resolve, 1;

subtest 'cancel' => sub {
    my $resolver = new UniEvent::Resolver();
    my $i;
    my $req = $resolver->resolve({
        node       => 'ya.ru',
        on_resolve => sub {
            my ($addr, $err, $req) = @_;
            is $err->code, ECANCELED;
            $i++;
        },
    });
    
    $req->cancel;
    is $i, 1;
    
    $l->run;
};

sub test_resolve {
    my $cached = shift;
    my $resolver = new UniEvent::Resolver();
    my $host = "ya.ru";
    
    my $i = 0;
    
    $resolver->resolve({
        node       => $host,
        use_cache  => $cached,
        on_resolve => sub {
            my ($addr, $err, $req) = @_;
            ok !$err;
            ok $addr, "@$addr";
            ok $req;
            $i++;
        },
    });
    
    $resolver->resolve({
        node       => 'localhost',
        use_cache  => $cached,
        hints      => {family => UniEvent::AF_INET},
        on_resolve => sub {
            my ($addr, $err, $req) = @_;
            ok !$err;
            ok $addr, "@$addr";
            ok $req;
            is $addr->[0]->ip, "127.0.0.1";
            $i += 2;
        },
    });
    
    $l->run;
    
    is $i, 3;
    
    if ($cached) {
        $resolver->resolve({
            node       => $host,
            on_resolve => sub {
                my ($addr, $err, $req) = @_;
                ok !$err;
                $i += 10;
            },
        });
        $l->run_nowait;
        is $i, 13;
    }
}

done_testing();
