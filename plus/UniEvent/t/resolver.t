use 5.012;
use lib 't/lib';
use MyTest;

catch_run('[resolver]');

#my $l = UniEvent::Loop->default_loop();

#my $res;

#my @to_res = ('google-public-dns-a.google.com', 'domain');
#my ($check_ip, $check_port) = ('8.8.8.8', 53);

#sub get_sas {
    ## diag "WTF";
    #my $r = new UniEvent::Resolver;
    #my $sas;
    #$r->resolve_callback(sub {
        #(my $r, $sas, my $err) = @_;
        #$l->stop();
    #});
    #$r->resolve(@to_res);
    #$l->run();
    #return $sas;
#}

#my $t = new UniEvent::Timer;
#use constant INET_WAIT_TIME => 5;
#$t->start(INET_WAIT_TIME, sub {
    #$l->stop();
#});
#my $inet_present = 0;
#my $tcp = new UniEvent::TCP;
#$tcp->connect_callback(sub {
    #$inet_present = 1;
    #$l->stop();
#});
#$tcp->connect($check_ip, $check_port);
#$l->run();

#if ($inet_present) {
    #my $sa = get_sas()->[0];
    #is($sa->ip, $check_ip, 'Google public DNS IP resolution');
    #is($sa->port, $check_port, 'Google public DNS port resolution');
#}
#else {
    #plan skip_all => 'No Internet!';
#}

done_testing();
