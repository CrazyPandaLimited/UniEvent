use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent qw/:const
    cpu_info resident_set_memory uptime free_memory total_memory hrtime interface_info get_rusage loadavg
    inet_pton inet_ntop inet_ptos inet_stop inet_sockaddr_info hostname
/;

# check constants
cmp_ok(
    AF_INET + AF_INET6 + INET_ADDRSTRLEN + INET6_ADDRSTRLEN + PF_INET + PF_INET6,
    '>', 0, "constants exist"
);

# CPU INFO
{
    my ($cnt, @list);
    $cnt = cpu_info();
    cmp_ok($cnt, '>', 0, "cpu_info: we have processors");
    ($cnt, @list) = cpu_info();
    is($cnt, scalar @list, "cpu_info: detailed info for all processors exists");
    ok(defined $list[0]->{model}, "cpu_info: model");
    ok(defined $list[0]->{speed}, "cpu_info: speed");
    ok($list[0]->{cpu_times}, "cpu_info: cpu_times");
    ok(defined $list[0]->{cpu_times}{user}, "cpu_info: user");
    ok(defined $list[0]->{cpu_times}{nice}, "cpu_info: nice");
    ok(defined $list[0]->{cpu_times}{sys}, "cpu_info: sys");
    ok(defined $list[0]->{cpu_times}{idle}, "cpu_info: idle");
    ok(defined $list[0]->{cpu_times}{irq}, "cpu_info: irq");
}

# resident_set_memory
my $rss = resident_set_memory();
cmp_ok($rss, '>', 0, "resident set memory exists");
my %aaa = map {$_ => $_} 1..100000;
cmp_ok(resident_set_memory(), '>', $rss, "resident set memory works");

# uptime
cmp_ok(uptime(), '>', 0, "uptime works");

# free_memory
cmp_ok(free_memory(), '>', 0, "free_memory works");

# total_memory
cmp_ok(total_memory(), '>', free_memory(), "total_memory works");

# hrtime
my $hrtime = hrtime();
cmp_ok(hrtime(), '>', $hrtime, "hrtime works");

# inet_pton/ntop
foreach my $row (
    ['0.0.0.0', 0],
    ['127.0.0.1', 16777343],
    ['192.168.1.1', 16885952],
    ['::1', 0,0,0,16777216],
    ['::0:0:0:0:1', 0,0,0,16777216],
    ['0:0:0:0:0:0:0:1', 0,0,0,16777216],
    ['abcd:0123:4567:8901:2345:6789:abcd:ef01', 587320747,25782085,2305246499,32492971],
    ['::ffff:240.239.238.237', 0,0,4294901760,3991859184],
) {
    my ($ip, @check) = @$row;
    my $unps = 'L' x @check;
    cmp_deeply([unpack($unps, inet_pton($ip))], \@check, "inet_pton IP=$ip");
    my $check_ip = $ip;
    $check_ip =~ s/:0([1-9])/:$1/g;
    $check_ip =~ s/^(0:)+/::/;
    $check_ip =~ s/^::(0:)+/::/;
    is(inet_ntop(inet_pton($ip)), $check_ip, "inet_ntop IP=$ip");
}
# inet_pton/ntop - bad addresses
foreach my $ip ("127.0.0", ".0.0.1.1", "1:1:1", "::1:") {
    ok(!eval{inet_pton($ip)}, "inet_pton bad address IP=$ip");
}

# inet_ptos/stop
foreach my $row (
    ["0.0.0.0", 1234],
    ["127.0.0.1", 0],
    ["192.168.1.1", 65535],
    ['::1', 1234],
    ['abcd:123:4567:8901:2345:6789:abcd:ef01', 5678],
    ['::ffff:240.239.238.237', 59999],
) {
    my ($ip, $port) = @$row;
    ok(inet_ptos($ip, $port), "inet_ptos IP=$ip PORT=$port");
    is(scalar inet_stop(inet_ptos($ip, $port)), $ip, "inet_stop(scalar) IP=$ip PORT=$port");
    cmp_deeply([inet_stop(inet_ptos($ip, $port))], [$ip, $port], "inet_stop(list) IP=$ip PORT=$port");
}
# inet_ptos/stop - bad arguments
foreach my $row (["127.0.0", 10], [".0.0.1.1", 10], ["1:1:1", 0], ["::1:", 0]) {
    my ($ip, $port) = @$row;
    ok(!eval{inet_ptos($ip, $port)}, "inet_ptos bad address IP=$ip PORT=$port");
}

# INTERFACE INFO
{
    my @list = interface_info();
    ok @list, "interface_info: interface_info() called ok";
    if (@list) {
        pass("interface_info: we have interfaces");
        ok(defined $list[0]->{name}, "interface_info: name");
        ok(defined $list[0]->{phys_addr}, "interface_info: phys_addr");
        ok(defined $list[0]->{is_internal}, "interface_info: is_internal");
        my $addr = inet_stop($list[0]->{address});
        my $nm   = inet_stop($list[0]->{netmask});
        ok(inet_pton($addr), "interface_info: address");
        ok(inet_pton($nm), "interface_info: netmask");
    }
    my @addresses = map { inet_stop($_->{address}) } @list;
    my $has_localhost = grep { $_ eq  '::1' || $_ eq '127.0.0.1'} @addresses;
    ok $has_localhost, "has local interface";
}

subtest "inet_sockaddr_info" => sub {
    subtest "v4" => sub {
        my $sock_addr = inet_ptos('127.0.0.1', 80);
        my $info = inet_sockaddr_info($sock_addr);
        is $info->{port}, 80;
        is $info->{ip}, '127.0.0.1';
        is $info->{family}, PF_INET;
    };

    subtest "v6" => sub {
        my $sock_addr = inet_ptos('fe80::f9e8:7226:b181:ea07', 65535);
        my $info = inet_sockaddr_info($sock_addr);
        is $info->{port}, 65535;
        is $info->{ip}, 'fe80::f9e8:7226:b181:ea07';
        is $info->{family}, PF_INET6;
    };
};

# get_rusage
my $rusage = get_rusage();
is(ref($rusage), 'HASH', 'get_rusage returns hashref');
my $rusage_sum = 0;
$rusage_sum += $rusage->{$_} for qw/utime stime maxrss ixrss idrss isrss minflt majflt nswap inblock oublock msgsnd msgrcv nsignals nvcsw nivcsw/;
cmp_ok($rusage_sum, '>', 0, "get_rusage returns some info");

# loadavg
my @avgs = loadavg();
is(scalar @avgs, 3, "loadavg returns 3 numbers");

#hostname
my $hostname = hostname();
ok($hostname, "hostname works");

done_testing();
