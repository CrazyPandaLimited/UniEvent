use strict;
use warnings;
use lib 't'; use PETest;
use UniEvent::Error;
use UniEvent::Pipe;
use Test::More;

package main;

my $l = UniEvent::Loop->default_loop;

my $acceptor = new UniEvent::Pipe;
use constant PIPE_PATH => PETest::var 'pipe';
$acceptor->bind(PIPE_PATH);
$acceptor->listen();
my $p = new UniEvent::Prepare;

TODO: {
    ok(test_serv_read_shuffling(), "Server reads shuffling");
}

sub test_serv_read_shuffling {
    use Devel::Peek;
    my $singletone_client;
    $acceptor->connection_callback(sub {
	    my ($acceptor, $err) = @_;
	    diag $err if $err;
	    if (!$singletone_client) {
    		my $client = new UniEvent::Pipe;
            $singletone_client = $client;
    		$acceptor->accept($client);
            $client->read_start;
    		$client->eof_callback(sub {
    			#diag "EOF callback started";
    			undef $singletone_client;
    			$acceptor->weak(1);
            });
            $client->shutdown_callback(sub {
            	#diag "server on_shutdown @_";
            });
    		#diag "Issuing shutdown() now!";
    		$client->shutdown();
	    }
	});
    $p->start(sub {
	    my $cl = new UniEvent::Pipe;
	    $cl->connect(PIPE_PATH);
	    $cl->shutdown();
	    $cl->shutdown_callback(sub {
	    	#diag "client on_shutdown @_";
	    });
	    $p->stop();
	});
    $l->run();
    #diag "That's o'kay";
    return !$singletone_client;
}

done_testing();