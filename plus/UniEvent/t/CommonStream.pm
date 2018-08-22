use strict;
package CommonStream;
use lib 't';
use Scalar::Util qw/blessed/;
use Test::More;
use UniEvent;
use Binder;
use Carp;

# use constant OK_TOKEN => 'OK';

sub run_now {
    my $action = $_[1];
    my $h = $_[0];
    $h->asyncq_run(
	sub {
	    $action->($_[0]);
	    $h->loop->stop();
	}
       );
    $h->loop->run();
}

sub make_binder {
    my $binder = $_[0];
    my $port_ref = $_[1];
    return sub {
	my $h = $_[0];
	$$port_ref = Binder::bind2free(
	    sub {
		$binder->($h, $_[0]);
		$h->listen();
	    }
	   );
    };
}

sub regular_bind {
    my $tcp = $_[0];
    my $port;
    my $sub = make_binder(
	sub {
	    $_[0]->bind(UniEvent::inet_ptos('127.0.0.1', $_[1]));
	},
	\$port);
    run_now($tcp, $sub);
    return $port;
}

sub concurrent_sub {
    my $sub = shift;
    my @args = @_;
    my $pid = fork;
    if (!$pid) {
        eval { $sub->(@_) };
        exit 0;
    }
    return $pid;
}

sub make_checker {
    my ($token, $pkg) = @_;
    my $status;
    my $client = new $pkg;
    my $built_str;
    $client->read_callback(
	sub {
	    # diag "HS";
	    my ($h, $str, $err) = @_;
	    # diag 'HOORAY';
	    $built_str .= $str;
	});
    $client->eof_callback(
	sub {
	    # diag '$built_str: '.$built_str;
	    $_[0]->disconnect();
	    $status = $token eq $built_str;
	    $_[0]->loop->stop();
	});
    return ($client, \$status);
}

sub to_listener {
    my ($tcp, $client, $on_accept) = @_;
    $tcp->weak(0);
    $tcp->connection_callback(
	sub {
	    eval {
            #diag 'Got connection!!! HOORAY!!! :-)';
			my ($h, $err) = @_;
			die "Some error ($err) in accepting socket" if $err;
			$h->accept($client);
			$on_accept->($client);
			$h->weak(1);
	    }
	});
}

sub test_serv_reading {
    my ($tcp, $initer, $mag_tok, $sub) = @_;
    my ($client, $status) = make_checker($mag_tok, blessed $tcp);
    to_listener($tcp, $client, sub {});
    my $port = $initer->($tcp);
    # $tcp->connection_callback()->();
    my $p = new UniEvent::Prepare;
    $p->start(
	sub {
	    my $p = $_[0];
            concurrent_sub($sub, $port, $mag_tok);
	    $p->stop();
	});
    eval { $tcp->loop->run(); 1 } or diag "Fucked up: $@";
    return $$status;
}

sub test_serv_writing {
    my ($tcp, $initer, $mag_tok, $sub) = @_;
    my ($client, $status) = make_checker($mag_tok, blessed $tcp);
    to_listener(
	$tcp, $client,
	sub {
	    $_[0]->write($mag_tok, sub {$_[0]->shutdown()});
	});
    my $port = $initer->($tcp);
    my $p = new UniEvent::Prepare;
    $p->start(
	sub {
	    concurrent_sub($sub, $port, $mag_tok);
	    $_[0]->stop();
	});
    eval { $tcp->loop->run(); 1 } or diag $@;
    return $$status;
}

1;
