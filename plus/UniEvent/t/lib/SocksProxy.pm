package SocksProxy;
use 5.020;
use warnings;
use UniEvent;
use IO::Select;
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

    die "ERROR: Can not bind to $host:$port" unless $server;

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

sub start {
    my (%args) = @_;
    my $host = $args{host} // "127.0.0.1";
    my $port = $args{port} // UniEvent::find_free_port();
    my $auth = $args{auth} // 1;
    my $user = $args{user} // "user";
    my $pass = $args{pass} // "pass";
    
    local $SIG{HUP} = sub { warn "EBANAROT" };
 
    my $pid = fork();
    if ($pid) {
        MyTest::variate_socks_url($auth ? "socks5://$user:$pass\@$host:$port" : "socks5://$host:$port");
        select undef, undef, undef, 0.05;
        return $pid;
    }
    die "could not fork" unless defined $pid;
    
    alarm(0);
    my $server = set_server($host, $port, $auth, sub { return $_[0] eq $user && $_[1] eq $pass });
    my $selector = IO::Select->new($server);
    
    set_client($server) while 1;
    exit(0);
}

1;
