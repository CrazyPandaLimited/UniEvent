use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent;

subtest 'constants' => sub {
    cmp_ok(AF_INET + AF_INET6 + INET_ADDRSTRLEN + INET6_ADDRSTRLEN + PF_INET + PF_INET6, '>', 0);
};

subtest 'cpu info' => sub {
    my $cnt = UniEvent::cpu_info();
    cmp_ok $cnt, '>', 0, "we have $cnt processors";
    my @list = UniEvent::cpu_info();
    is $cnt, scalar @list, "detailed info for all $cnt processors exists";
    my $i = 0;
    foreach my $row (@list) { subtest 'CPU '.($i++) => sub {
        ok defined $row->{model}, "model $row->{model}";
        ok defined $row->{speed}, "speed $row->{speed}";
        ok defined $row->{cpu_times}{user}, "user $row->{cpu_times}{user}";
        ok defined $row->{cpu_times}{nice}, "nice $row->{cpu_times}{nice}";
        ok defined $row->{cpu_times}{sys},  "sys $row->{cpu_times}{sys}";
        ok defined $row->{cpu_times}{idle}, "idle $row->{cpu_times}{idle}";
        ok defined $row->{cpu_times}{irq},  "irq $row->{cpu_times}{irq}";
    }}
};

## resident_set_memory
#my $rss = resident_set_memory();
#cmp_ok($rss, '>', 0, "resident set memory exists");
#my %aaa = map {$_ => $_} 1..100000;
#cmp_ok(resident_set_memory(), '>', $rss, "resident set memory works");
#
## uptime
#cmp_ok(uptime(), '>', 0, "uptime works");
#
## free_memory
#cmp_ok(free_memory(), '>', 0, "free_memory works");
#
## total_memory
#cmp_ok(total_memory(), '>', free_memory(), "total_memory works");
#
## hrtime
#my $hrtime = hrtime();
#cmp_ok(hrtime(), '>', $hrtime, "hrtime works");
#
## INTERFACE INFO
#{
#    my @list = interface_info();
#    ok @list, "interface_info: interface_info() called ok";
#    if (@list) {
#        pass("interface_info: we have interfaces");
#        ok(defined $row->{name}, "interface_info: name");
#        ok(defined $row->{phys_addr}, "interface_info: phys_addr");
#        ok(defined $row->{is_internal}, "interface_info: is_internal");
#        ok($row->{address}->ip, "interface_info: address");
#        ok($row->{netmask}->ip, "interface_info: netmask");
#    }
#    my @addresses = map { $_->{address}->ip } @list;
#    my $has_localhost = grep { $_ eq  '::1' || $_ eq '127.0.0.1'} @addresses;
#    ok $has_localhost, "has local interface";
#}
#
## get_rusage
#my $rusage = get_rusage();
#is(ref($rusage), 'HASH', 'get_rusage returns hashref');
#my $rusage_sum = 0;
#$rusage_sum += $rusage->{$_} for qw/utime stime maxrss ixrss idrss isrss minflt majflt nswap inblock oublock msgsnd msgrcv nsignals nvcsw nivcsw/;
#cmp_ok($rusage_sum, '>', 0, "get_rusage returns some info");
#
## loadavg
#my @avgs = loadavg();
#is(scalar @avgs, 3, "loadavg returns 3 numbers");
#
##hostname
#my $hostname = hostname();
#ok($hostname, "hostname works");

done_testing();
