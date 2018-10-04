use strict;
use warnings;
use Test::More;
use lib 't'; use PETest;
use UniEvent::Error;
use UniEvent::UDP;
use Binder;
use Scalar::Util qw/weaken/;
use Socket qw(PF_INET IPPROTO_UDP SOMAXCONN SOCK_DGRAM inet_aton);
use Binder;
use IPC::Open2;

use constant ADDR_N => 2**8;

my $base_addr = $ENV{'UNIEVENT_TEST_MULTICAST_BASE_ADDR'};

my @primes = (
    2, 3, 5, 7, 11, 13, 17, 19
   );

my %msgs;

sub gen_msgs {
    my ($primes) = @_;
    my $msgs = {0 => 1};
    for my $prime (@$primes) {
	my %new_msgs;
	for (keys %$msgs) {
	    my $prod = $msgs->{$_};
	    my $skip = 2 * $_;
	    my $use = $skip + 1;
	    $new_msgs{$skip} = $prod;
	    $new_msgs{$use} = $prod * $prime;
	}
    }
}

plan skip_all => "Tests are not implemented";
# unless ($base_addr) {
#     plan skip_all => "Not intended to run unless explicitly requested by user through defining UNIEVENT_TEST_MULTICAST_BASE_ADDR";
# }

my $l = UniEvent::Loop->default_loop;
my $p = new UniEvent::Prepare;

fail("Multicast");

done_testing();
