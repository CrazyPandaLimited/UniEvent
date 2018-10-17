use 5.012;
use lib 't/lib';
use MyTest;
use CommonStream;
use Net::SSLeay;

sub sslerr () {
    die Net::SSLeay::ERR_error_string(Net::SSLeay::ERR_get_error);
}

use constant SERV_CERT => "t/cert.pem";

my $serv_ctx = Net::SSLeay::CTX_new();

Net::SSLeay::CTX_use_certificate_file($serv_ctx, SERV_CERT, &Net::SSLeay::FILETYPE_PEM) or sslerr();

Net::SSLeay::CTX_use_PrivateKey_file($serv_ctx, "t/key.pem", &Net::SSLeay::FILETYPE_PEM) or sslerr();
Net::SSLeay::CTX_check_private_key($serv_ctx) or sslerr();
my $client_ctx = Net::SSLeay::CTX_new();
my $res = Net::SSLeay::CTX_load_verify_locations($client_ctx, SERV_CERT, '');
die "something went wrong" unless $res;

my $tcp = new UniEvent::TCP;
my $port = CommonStream::regular_bind($tcp);
$tcp->use_ssl($serv_ctx);

sub test_self_pleasing_1 {
    my ($tcp, $mag_tok) = @_;
    my $initiator_recvd, my $echo_recvd;
    my $echo = new UniEvent::TCP;
    my $p = new UniEvent::Prepare;
   
    my $client; 
    my $srv;
    my $err;
    $tcp->weak(0);
    $tcp->connection_callback(sub {
        ($srv, $client, $err) = @_;
         #diag "connected";

        $client->write($mag_tok, sub {
             #diag 'write finished';
            $client->shutdown();
             #diag 'shutdown does not croak';
        });
        
        $client->read_start(sub {
            my ($h, $str, $err) = @_;
             #diag "initiator read: $str";
            $initiator_recvd .= $str;
        });
        
        $client->eof_callback(sub {
             #diag "client disconnected";
            $_[0]->loop->stop();
        });
        $tcp->weak(1);
    });
        
    
    $p->start(sub {
        $_[0]->stop();
        
        $echo->use_ssl($client_ctx);
        
        $echo->read_start(sub {
            my ($h, $str, $err) = @_;
             #diag "read : $str";
            $h->write($str, sub {
                 #diag "write";
            });
            $echo_recvd .= $str;
            return 1;
        });
        
        $echo->eof_callback(sub {
             #diag "server disconnected";
            $ok = $echo_recvd eq $mag_tok;
            $_[0]->shutdown();
        });
        
        $echo->connect_callback(sub {
        	my (undef, $err) = @_;
            if ($err) {
                 diag "err: $err";
            } else {
                 #diag "connected : client"
            }
        });
       
         #diag "echo connect"; 
        $echo->connect('127.0.0.1', $port);
    });
    
    eval { $tcp->loop->run(); 1 } or diag "Loop failed with: $@";
    
    return ($initiator_recvd, $echo_recvd);
}

sub test_self_pleasing_2 {
    my ($tcp, $mag_tok) = @_;
    my $initiator_recvd, my $echo_recvd;
    my $echo = new UniEvent::TCP;
    my $initiator = new UniEvent::TCP;
    my $p = new UniEvent::Prepare;
    $initiator->connection_callback(
        sub {
            # diag 'Connected : server';
        }
       );
    $initiator->read_callback(
        sub {
            my ($h, $str, $err) = @_;
            $initiator_recvd .= $str;
        });
    $initiator->eof_callback(
        sub {
            $_[0]->loop->stop();
        });
    CommonStream::to_listener(
        $tcp, $initiator,
        sub {
            # diag 'OK';
            $_[0]->write(
                $mag_tok,
                sub {
                });
            $_[0]->shutdown();
        });
    $p->start(
        sub {
            $echo->use_ssl($client_ctx);
            $echo->read_callback(
                sub {
                    my ($h, $str, $err) = @_;
                    # diag "Read : $str";
                    $h->write(
                        $str, sub {
                            # diag "George said to put this here!";
                        });
                    $echo_recvd .= $str;
                    return 1;
                });
            $echo->eof_callback(
                sub {
                    $ok = $echo_recvd eq $mag_tok;
                    $_[0]->shutdown();
                });
            $echo->connect('127.0.0.1', $port);
            $echo->connect_callback(
                sub {
                    my $err = $_[2];
                    if ($err) {
                        diag "ERR: $err";
                    }#  else {
                    #     diag "Connected : client"
                    # }
                });
            $_[0]->stop();
        });
    eval { $tcp->loop->run(); 1 } or diag "Loop failed with: $@";
    return ($initiator_recvd, $echo_recvd);
}

my $magic_ssl = 'MAGIC SSL';
my ($initiator_recvd, $echo_recvd);

($initiator_recvd, $echo_recvd) = test_self_pleasing_1($tcp, $magic_ssl);
is($echo_recvd, $magic_ssl, 'SSL self pleasing conversation, $echo_recvd (no asyncq)');
is($initiator_recvd, $magic_ssl, 'SSL self pleasing conversation, $initiator_recvd (no asyncq)');

#($initiator_recvd, $echo_recvd) = test_self_pleasing_2($tcp, $magic_ssl);
#is($echo_recvd, $magic_ssl, 'SSL self pleasing conversation, $echo_recvd (asyncq)');
#is($initiator_recvd, $magic_ssl, 'SSL self pleasing conversation, $initiator_recvd (asyncq)');

# ok(test_self_pleasing_2($tcp, $magic_ssl), "SSL self pleasing conversation (no asyncq on server)");
done_testing();
