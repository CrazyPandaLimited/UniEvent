use 5.012;
use warnings;

use Test::More;
use UniEvent;
use Benchmark qw/timethese/;
use IO::Select;
use IO::Socket::Socks;
use IO::Socket::Socks ':constants';

plan skip_all => 'rebuild Makefile.PL adding TEST_FULL=1 to enable all tests' unless UniEvent->can('_run_cpp_tests_');

my $use_local_proxy = 1;
my $use_ssl = 1;
my $username = "user";
my $password = "pass";

my $proxy_resolve = 1;

my $server_ip = "127.0.0.1";
my $server_port = 13924;

my $proxy_ip;
my $proxy_port;
if($use_local_proxy) {
    $proxy_ip = $server_ip;
    $proxy_port = $server_port;
} else {
    # insert some alive proxy
    $proxy_ip = "94.130.211.41";
    $proxy_port = 1080;
}

my $target_host;
my $target_port;
if($use_ssl) {
    $target_host = "www.google.com";
    $target_port = "443";
} else {
    $target_host = "google.com";
    $target_port = "80";
}

sub set_server {
    my $server_ip = shift;
    my $server_port = shift;

    my $server = new IO::Socket::Socks(
        SocksResolve    => 1,
        ProxyAddr	=> $server_ip,
        ProxyPort	=> $server_port,
        Listen		=> 1,
        UserAuth	=> \&server_auth,
        RequireAuth	=> $username ? 1 : 0,
        Timeout		=> 5,
        ReuseAddr       => 1,
    );

    unless ($server) {
        warn 'ERROR: Can not bind to ' . $server_ip . ':' . $server_port;
        exit(0);
    }

    return $server;
}

sub server_auth {
    my $user = shift;
    my $pass = shift;
    return $user eq $username && $pass eq $password;
}

sub set_client {
    my $server = shift;

    my $client = $server->accept;
    return unless $client;

    my ($command, $host, $port) = @{$client->command()};
    if ($command && $command == CMD_CONNECT) {
        my $socks = IO::Socket::INET->new(PeerHost => $host, PeerPort =>$port , Timeout => 5);
        unless ($socks) {
            $client->close();
            return;
        }

        $client->command_reply(0, $socks->sockhost, $socks->sockport);
        my $selector = IO::Select->new($socks, $client);
        my $ok = 1;

        while($ok) {
            my @ready = $selector->can_read();
            foreach my $s (@ready) {
                my $data;
                $s->recv($data, 4096);
                
                unless($data) {
                    $selector->remove($socks);
                    $socks->close();
                    $ok = 0;
                    last;
                }

                if($s == $socks) {
                    $client->send($data);
                } else {
                    $socks->send($data);
                }
            }
        }

        $selector->remove($client);
    }
    $client->close();
}

my $pid = fork();
    die if not defined $pid;

unless($pid) {
    my $server = set_server($server_ip, $server_port);
    my $selector = IO::Select->new($server);

    while (my $cc = $selector->can_read(10)) {
        set_client($server);
    }

    warn "exit";
    exit(0);
}

sleep(1);

$ENV{"PANDA_EVENT_PROXY"} = "socks5://$username:$password\@$proxy_ip:$proxy_port"; 
$ENV{"PANDA_EVENT_PROXY_RESOLVE"} = $proxy_resolve; 

ok (UniEvent::_run_cpp_tests_('[timeout]'));

kill 'KILL', $pid;

done_testing();


