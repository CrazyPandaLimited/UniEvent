use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Signal qw/:const signame/;

my ($l, $s, $t, $err, $lastsignum);
my @constants = qw/
    SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGBUS SIGFPE SIGKILL SIGUSR1 SIGSEGV SIGUSR2 SIGPIPE SIGALRM SIGTERM
    SIGCHLD SIGCONT SIGSTOP SIGTSTP SIGTTIN SIGTTOU SIGURG SIGXCPU SIGXFSZ SIGVTALRM SIGPROF SIGWINCH SIGSYS
/;

for my $cname (sort @constants) {
    no strict 'refs';
    my $f = \&{"UniEvent::Signal::$cname"};
    ok(defined $f->(), "constant $cname exists");
}

$l = UniEvent::Loop->default_loop;
$s = new UniEvent::Signal;
$t = new UniEvent::Timer;

$s->signal_callback(\&cb);
$t->start(0.001);

foreach my $signum (SIGHUP, SIGINT, SIGUSR1, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD) {
    $s->start($signum);
    is($s->signum, $signum, "signal has been bound to correct signum ($signum)");
    is($s->signame, signame($signum), 'signame/signame_for return correct values: '.$s->signame);
    $t->timer_callback(sub { kill $signum => $$});
    $l->run;
    $s->stop;
    is($lastsignum, $signum, "signal handler called for ".$s->signame($signum));
    
    $s->start_once($signum);
    $l->run;
    is($lastsignum, $signum, "signal handler called again for ".$s->signame($signum));
}

sub cb {
    my ($h, $signum) = @_;
    $lastsignum = $signum;
    $l->stop;
}

done_testing();
