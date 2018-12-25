use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
BEGIN { plan skip_all => 'disabled'; }
use UniEvent qw/:const
    cpu_info resident_set_memory uptime free_memory total_memory hrtime interface_info get_rusage loadavg hostname
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

# INTERFACE INFO
{
    my @list = interface_info();
    ok @list, "interface_info: interface_info() called ok";
    if (@list) {
        pass("interface_info: we have interfaces");
        ok(defined $list[0]->{name}, "interface_info: name");
        ok(defined $list[0]->{phys_addr}, "interface_info: phys_addr");
        ok(defined $list[0]->{is_internal}, "interface_info: is_internal");
        ok($list[0]->{address}->ip, "interface_info: address");
        ok($list[0]->{netmask}->ip, "interface_info: netmask");
    }
    my @addresses = map { $_->{address}->ip } @list;
    my $has_localhost = grep { $_ eq  '::1' || $_ eq '127.0.0.1'} @addresses;
    ok $has_localhost, "has local interface";
}

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
