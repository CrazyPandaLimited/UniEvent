use strict;
use warnings;
use lib 't/lib'; use MyTest;
use Net::SockAddr;
plan skip_all => 'disabled';
my $loop = UniEvent::Loop::default_loop();

my $server = UniEvent::TCP->new();
my $cl;
my $pid;
$server->tcp_nodelay(1);
$server->bind(SA_LOOPBACK_ANY);
$server->listen(8);
my $sa = $server->get_sockaddr;

$server->connection_callback(sub {
    my ($srv, $cl, $err) = @_;
    die $err if $err;
    ok(1, "server: connection");
    
    $cl->read_start(sub {
        my ($handle, $data, $err) = @_;
        die $err if $err;
        is($data, "client_data", "server: read"); #1
        $cl->write("data");
    });
    
    $cl->eof_callback(sub {
        $loop->stop;
    });
});

my $timer = UniEvent::Timer->new;
$timer->timer_callback(sub {
    $pid = fork();
    unless ($pid) {
        $loop->handle_fork();
        
        $server->reset();
        
        my $client = UniEvent::TCP->new();
        $client->tcp_nodelay(1);
        
        $client->connect_callback(sub {
            my ($handle, $err) = @_;
            die $err if $err;
            ok(1, "child: connect");
            $handle->write("client_data");
        });
        
        $client->read_start(sub {
            my ($handle, $data, $err) = @_;
            die $err if $err;
            is($data, "data", "child: read");
            $client->shutdown_callback(sub {
                $loop->stop;
            });
            $client->shutdown;
        });
        
        $client->connect($sa);
    }
});
$timer->start(0, 0.01);

$loop->run;

exit unless $pid;
waitpid($pid, 0);
done_testing(4);