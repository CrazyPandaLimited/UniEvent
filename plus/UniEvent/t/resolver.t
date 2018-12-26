use 5.012;
use lib 't/lib';
use MyTest;
BEGIN { plan skip_all => 'disabled'; }
catch_run('[resolver]');

my $l = UniEvent::Loop->default_loop();

sub test_cached_resolver { 
    my ($check_ip, $check_port) = ('8.8.8.8', 53);
    my $r = new UniEvent::Resolver($l);
    my $sa1;
    # not in cacne, async call
    $r->resolve('google-public-dns-a.google.com', 'domain', sub { (my $r, $sa1, my $err) = @_; }, {"family" => UniEvent::AF_INET});
    
    $l->run();
    
    is($sa1->[0]->ip, $check_ip, 'Google public DNS IP resolution');
    is($sa1->[0]->port, $check_port, 'Google public DNS port resolution');
    
    my $sa2;
    # in cache, sync call
    $r->resolve('google-public-dns-a.google.com', 'domain', sub { (my $r, $sa2, my $err) = @_; }, {"family" => UniEvent::AF_INET});
    
    is($sa2->[0]->ip, $check_ip, 'Google public DNS IP resolution');
    is($sa2->[0]->port, $check_port, 'Google public DNS port resolution');
}

test_cached_resolver();

done_testing();
