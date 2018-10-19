use 5.020;
use warnings;

use Test::More;
use UniEvent;
use IO::Socket::Socks;
use IO::Select;
use IO::Socket::Socks;
use IO::Socket::Socks ':constants';

use lib 't/lib';
use SocksTest;

my $proxy_ip = "127.0.0.1";
my $proxy_port = UniEvent::find_free_port();

my $pid = SocksTest::socks_test(host => $proxy_ip, port => $proxy_port, auth => 0, resolve => 1);

my $hints = UniEvent::addrinfo_hints(SOCK_STREAM, PF_INET);
my $loop = UniEvent::Loop->default_loop;

my $cl = new UniEvent::TCP();

$cl->use_socks($proxy_ip, $proxy_port);
$cl->connect("google.com", "80", 1, $hints, sub {
    my ($handler, $err) = @_;
    ok !$err;
    fail $err if $err;
    #note "connected";
});

$cl->write("GET / HTTP/1.1\r\nHost: www.google.com\r\nUser-Agent: TestAgent\r\nConnection: close\r\n\r\n");

my $result;

$cl->read_start(
    sub {
        #note "read";
        my ($c, $str, $err) = @_;
        $result .= $str;
    }
);
		
$cl->eof_callback(
    sub {
        #note "eof";
        $loop->stop();
        #print $result . "\n";
    }
);

$loop->run();

#print $cl->dump() . "\n";

my $finished = wait();

done_testing();
