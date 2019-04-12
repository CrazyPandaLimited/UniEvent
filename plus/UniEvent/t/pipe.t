use strict;
use warnings;
use lib 't/lib'; use MyTest;

use constant PIPE_PATH => MyTest::var 'pipe';

my $l = UniEvent::Loop->default_loop;

my $acceptor = new UniEvent::Pipe;
$acceptor->bind(PIPE_PATH);
$acceptor->listen();
my $p = new UniEvent::Prepare;

TODO: {
    ok(test_serv_read_shuffling(), "Server reads shuffling");
}

sub test_serv_read_shuffling {
    use Devel::Peek;
    my $client;
    $acceptor->connection_factory(sub {
        return $client ||= new UniEvent::Pipe;
    });
    $acceptor->connection_callback(sub {
        my ($srv, undef, $err) = @_;
        #diag "Connection";
        $client->read_start;
        $client->eof_callback(sub {
            #diag "EOF callback started";
            undef $client;
            $acceptor->weak(1);
        });
        #diag "Issuing shutdown() now!";
        $client->shutdown();
    });
    $p->start(sub {
	    my $cl = new UniEvent::Pipe;
	    $cl->connect(PIPE_PATH, sub {
	        #diag "on_connect";
            die $_[1] if $_[1];
	    });
	    $cl->shutdown(sub {
            #diag "client on_shutdown";
            die $_[1] if $_[1];
        });
	    $p->stop();
	});
    $l->run();
    #diag "That's o'kay";
    return !$client;
}

done_testing();