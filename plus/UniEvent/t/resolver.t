use 5.012;
use lib 't/lib';
use MyTest;

catch_run('[resolver]');

my $l = UniEvent::Loop->default_loop();

#subtest 'not cached' => sub {
#    my $resolver = new UniEvent::Resolver();
#    
#    $resolver->resolve({
#        node      => 'localhost',
#        use_cache => 0,
#        on_resolve => sub {
#            my ($resolver, $addr, $err) = @_;
#            die "CB";
#            ok !$err;
#            ok $addr;
#            use Data::Dumper;
#            warn Dumper($addr);
#        },
#    });
#    
#    warn "AFTER RES CALL";
#    
#    eval {
#        $resolver = undef;
#    };
#    
#    warn "RES UNDEF";
#};

#sub test_cached_resolver { 
#    my ($check_ip, $check_port) = ('8.8.8.8', 53);
#    my $r = new UniEvent::Resolver($l);
#    my $sa1;
#    # not in cacne, async call
#    $r->resolve('google-public-dns-a.google.com', 'domain', sub { (my $r, $sa1, my $err) = @_; }, {"family" => UniEvent::AF_INET});
#    
#    $l->run();
#    
#    is($sa1->[0]->ip, $check_ip, 'Google public DNS IP resolution');
#    is($sa1->[0]->port, $check_port, 'Google public DNS port resolution');
#    
#    my $sa2;
#    # in cache, sync call
#    $r->resolve('google-public-dns-a.google.com', 'domain', sub { (my $r, $sa2, my $err) = @_; }, {"family" => UniEvent::AF_INET});
#    
#    $l->run_nowait();
#
#    is($sa2->[0]->ip, $check_ip, 'Google public DNS IP resolution');
#    is($sa2->[0]->port, $check_port, 'Google public DNS port resolution');
#}
#
#test_cached_resolver();

done_testing();
