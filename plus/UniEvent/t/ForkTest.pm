package ForkTest;
use 5.012;
use warnings;
use UniEvent;
use UniEvent::Loop;
use UniEvent::Error;
use UniEvent::TCP;
use Test::More;
use Scalar::Util qw/weaken/;
use Socket qw(PF_INET SOMAXCONN SOCK_STREAM INADDR_ANY sockaddr_in inet_aton);

sub start_fork_test {

	my $test_target = shift;

	my $proc = "master";
	my $loop = UniEvent::Loop::default_loop();

	my $server = UniEvent::TCP->new();
	my $cl;
	my $pid;
	$server->tcp_nodelay(1);
	my $serverSocket = sockaddr_in (0, inet_aton("0.0.0.0"));
	$server->bind($serverSocket);
	$serverSocket = $server->getsockname;
	$server->connection_callback(sub {
	    #say "SERVER CONNECTION CB";
		$cl = UniEvent::TCP->new();
		$cl->read_start(sub {
			my ($handle, $data) = @_;
			#say "SERVER READ GOT $data";
			if ($test_target eq "master") {
				is($data, "client_data", "server read"); #1
				is($proc, "master", "master connection"); #2
			}
			$cl->write("data");
		});

		$cl->eof_callback(sub {
			$loop->stop();
		});

		$server->accept($cl);
	});

	$server->listen(8);

	my $timer_d = UniEvent::Timer->new();
	$timer_d->timer_callback(sub {
		#print($proc.": timer $$\n");
		$pid = fork();
		unless ($pid) {
			$proc = "child";
			$loop->handle_fork();

			$server->reset();

			my $client = UniEvent::TCP->new();
			$client->tcp_nodelay(1);
			$client->connect_callback(sub {
			    #say "CONNECT CB";
				my ($handle, $err) = @_;
				if ($err) {
					print ($proc.": error:$err\n");
				} else {
					if ($test_target eq "child") {
						is($proc, "child", "child connect");
					}
					$handle->write("client_data");
				}
			});
			$client->read_start(sub {
				my ($handle, $data) = @_;
				if ($test_target eq "child") {
					is($data, "data", "child read");
				}
				$client->shutdown_callback(sub {
					$loop->stop();
				});
				$client->shutdown();
			});
			$client->connect($serverSocket);
		}
	});
	$timer_d->start(0, 0.01);

	$loop->run();

	if ($pid) {
		waitpid($pid, 0);
		done_testing(2) if ($test_target eq "master");
	}
    else {
        done_testing(2) if ($test_target eq "child");
    }
}

1;
