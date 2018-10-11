use strict;
use lib 't/lib';
use MyTest;

my $l = UniEvent::Loop->default_loop();

my $res;

my @to_res = ('google-public-dns-a.google.com', 'domain');
my ($check_ip, $check_port) = ('8.8.8.8', 53);

# sub num_gni {
#     my $sa = $_[0];
#     my ($err, $h, $s) = Socket::getnameinfo $sa, Socket::NI_NUMERICHOST | Socket::NI_NUMERICSERV;
#     diag $h;
#     diag $s;
#     return ($h, $s);
# }

# my ($gai_err, @gai_res) = Socket::getaddrinfo @to_res, {family => Socket::AF_INET};
# diag Dumper($gai_err);
# diag Dumper(@gai_res);
# diag "Real ones (Socket):";
# map {num_gni $_->{addr}} @gai_res;

sub get_sas {
    # diag "WTF";
    my $r = new UniEvent::Resolver;
    my $sas;
    $r->resolve_callback(
        sub {
            (my $r, $sas, my $err) = @_;
            $l->stop();
        });
    $r->resolve(@to_res);
    $l->run();
    return $sas;
}

my $t = new UniEvent::Timer;
use constant INET_WAIT_TIME => 5;
$t->start(
    INET_WAIT_TIME,
    sub {
        $l->stop();
    });
my $inet_present = 0;
my $tcp = new UniEvent::TCP;
$tcp->connect_callback(
    sub {
        $inet_present = 1;
        $l->stop();
    });
$tcp->connect($check_ip, $check_port);
$l->run();

if ($inet_present) {
    my $sas = get_sas();
    my ($ip, $port) = UniEvent::inet_stop($sas->[0]);
    is($ip, $check_ip, 'Google public DNS IP resolution');
    is($port, $check_port, 'Google public DNS port resolution');
}
else {
    plan skip_all => 'No Internet!';
}

done_testing();
