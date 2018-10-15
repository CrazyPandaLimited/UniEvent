use 5.020;
use warnings;

use Test::More;
use UniEvent;
use IO::Select;
use IO::Socket::Socks;
use IO::Socket::Socks ':constants';


my $use_local_proxy = 1;
my $use_ssl = 1;
my $username = "";
my $password = "";

my $proxy_resolve = 1;

my $server_ip = "127.0.0.1";
my $server_port = 12400;

my $proxy_ip;
my $proxy_port;
if($use_local_proxy) {
    $proxy_ip = $server_ip;
    $proxy_port = $server_port;
} else {
    # insert some alive proxy
    $proxy_ip = "188.120.224.76";
    $proxy_port = 35184;
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

        warn 'client_IP=' . $client->peerhost() . '; host=' . $host . '; port: ' . $port;

        $client->command_reply(0, $socks->sockhost, $socks->sockport);
        my $selector = IO::Select->new($socks, $client);
        my $ok = 1;

        while($ok) {
            my @ready = $selector->can_read();
            foreach my $s (@ready) {
                my $data;
                $s->recv($data, 4096);
                
                note "got: " . length($data);

                unless($data) {
                    warn "closing";
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
    warn "closed";
}

my $pid = fork();
    die if not defined $pid;

unless($pid) {
    my $server = set_server($server_ip, $server_port);
    my $selector = IO::Select->new($server);

    while (my $cc = $selector->can_read(2)) {
        set_client($server);
    }

    warn "exit";
    exit(0);
}

sleep(1);

my $hints = UniEvent::addrinfo_hints(SOCK_STREAM, PF_INET);
my $loop = UniEvent::Loop->default_loop;

my $cl = new UniEvent::TCP();

$cl->use_socks($proxy_ip, $proxy_port, $username, $password, $proxy_resolve);

if($use_ssl) {
    $cl->use_ssl();
}

$cl->connect($target_host, $target_port, 1, $hints, sub {
    my ($handler, $err) = @_;
    ok !$err;
    fail $err if $err;
    note "connected";
}, 1);

$cl->write("GET / HTTP/1.1\r\nHost: www.google.com\r\nUser-Agent: TestAgent\r\nConnection: close\r\n\r\n");

my $result;

$cl->read_start(
    sub {
        note "read";
        my ($c, $str, $err) = @_;
        $result .= $str;
    }
);
		
$cl->eof_callback(
    sub {
        note "eof";
        $loop->stop();

        #print $result . "\n";
    }
);

$loop->run();

print $cl->dump() . "\n";

my $finished = wait();

done_testing();
