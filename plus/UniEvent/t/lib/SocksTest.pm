package SocksTest;
use 5.020;
use warnings;

use Test::More;
use UniEvent;
use IO::Select;
use IO::Socket::Socks;
use IO::Socket::Socks ':constants';

sub set_server {
    my ($host, $port, $auth, $auth_sub) = @_;
    my $server = new IO::Socket::Socks(
        SocksResolve    => 1,
        ProxyAddr	=> $host,
        ProxyPort	=> $port,
        Listen		=> 1,
        UserAuth	=> $auth_sub,
        RequireAuth	=> $auth,
        Timeout		=> 2,
        ReuseAddr       => 1,
    );

    unless ($server) {
        warn "ERROR: Can not bind to $host:$port";
        exit(0);
    }

    return $server;
}

sub set_client {
    my $server = shift;

    my $client = $server->accept;
    return unless $client;

    my ($command, $host, $port) = @{$client->command()};
    if ($command && $command == CMD_CONNECT) {
        my $socks = IO::Socket::INET->new(PeerHost => $host, PeerPort =>$port , Timeout => 0.5);
        unless ($socks) {
            $client->close();
            return;
        }

        #warn 'client_IP=' . $client->peerhost() . '; host=' . $host . '; port: ' . $port;

        $client->command_reply(0, $socks->sockhost, $socks->sockport);
        my $selector = IO::Select->new($socks, $client);
        my $ok = 1;

        while($ok) {
            my @ready = $selector->can_read();
            foreach my $s (@ready) {
                my $data;
                $s->recv($data, 4096);
                
                #note "got: " . length($data);

                unless($data) {
                    #warn "closing";
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
    #warn "closed";
}

sub socks_test {
    my (%args) = @_;
    my $host = $args{host} // "127.0.0.1";
    my $port = $args{port} // UniEvent::find_free_port();
    my $auth = $args{auth} // 1;
    my $user = $args{user} // "user";
    my $pass = $args{pass} // "pass";
    my $resolve = $args{resolve} // 1;
 
    #print "$_ => $args{$_} " foreach (keys%args);
    #print "\n";

    my $pid = fork();
        die if not defined $pid;

    unless($pid) {
        my $server = set_server($host, $port, $auth, sub { return $_[0] eq $user && $_[1] eq $pass });
        my $selector = IO::Select->new($server);

        while (my $cc = $selector->can_read(2)) {
            set_client($server);
        }
        #warn "exit";
        exit(0);
    }

    if($auth) {
        $ENV{"PANDA_EVENT_PROXY"} = "socks5://$user:$pass\@$host:$port";
    } else {
        $ENV{"PANDA_EVENT_PROXY"} = "socks5://$host:$port";
    }

    $ENV{"PANDA_EVENT_PROXY_RESOLVE"} = $resolve; 

    sleep(1);

    return $pid;
}

1;
