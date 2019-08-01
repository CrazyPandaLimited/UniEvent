use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Error;
use UniEvent::Fs;

BEGIN { *Fs:: = *UniEvent::Fs:: }

my $call_cnt;
my $l = UniEvent::Loop->default_loop;

subtest 'non-existant file' => sub {
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.01);
    $h->callback(check_err(ENOENT, "watch non-existant file"));
    is($h->path, var 'file', "path getter works");
    $l->run;
    $l->run_nowait; # must be called only once
    check_call_cnt(1);
};

subtest 'file appears' => sub {
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.01);
    $h->callback(check_err(ENOENT, "watch non-existant file"));
    $l->run;
    check_call_cnt(1);
    $h->callback(check_appears("file appears"));
    Fs::touch(var 'file');
    $l->run;
    check_call_cnt(1);
    Fs::unlink(var 'file');
};

subtest 'mtime' => sub {
    Fs::touch(var 'file');
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.01);
    $h->callback(check_changes(STAT_MTIME, "file mtime"));
    my $t = UE::Timer->once(0.01, sub { Fs::touch(var 'file') });
    $l->run;
    check_call_cnt(1);
    Fs::unlink(var 'file');
};

subtest 'file contents' => sub {
    my $fd = Fs::open(var 'file', OPEN_RDWR | OPEN_CREAT);
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.01);
    $h->callback(check_changes([STAT_MTIME, STAT_SIZE], "file content"));
    my $t = UE::Timer->once(0.01, sub { Fs::write($fd, "epta") });
    $l->run;
    check_call_cnt(1);
    Fs::close($fd);
    Fs::unlink(var 'file');
};

subtest 'stop' => sub {
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.01);
    $h->stop;
    $h->callback(sub { $call_cnt++ });
    $l->run for 1..10;
    check_call_cnt(0);
    
    Fs::touch(var 'file');
    $h->start(var 'file', 0.1);
    my $t = UE::Timer->once(0.01, sub { $h->stop });
    $l->run for 1..10;
    check_call_cnt(0);
    
    Fs::unlink(var 'file');
};

subtest 'reset' => sub {
    Fs::touch(var 'file');
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.01);
    $h->callback(check_changes(STAT_MTIME, "mtime"));
    my $t = UE::Timer->once(0.01, sub { Fs::touch(var 'file') });
    $l->run;
    check_call_cnt(1);
    
    $h->reset;
    Fs::touch(var 'file');
    $l->run;
    check_call_cnt(0);
    
    Fs::unlink(var 'file');
};

subtest 'file remove' => sub {
    Fs::touch(var 'file');
    my $h = new UniEvent::FsPoll;
    $h->start(var 'file', 0.005);
    $l->run_nowait;
    select undef, undef, undef, 0.01;
    $l->run_nowait;
    $h->callback(check_err(ENOENT, "file remove"));
    my $t = UE::Timer->once(0.01, sub { Fs::unlink(var 'file') });
    $l->run;
    check_call_cnt(1);
};

done_testing();

sub check_err {
    my ($err_code, $name) = @_;
    return sub {
        my ($h, $prev, $curr, $err) = @_;
        return unless $err;
        is($err && $err->code, $err_code, "fspoll callback error code correct ($name)");
        $call_cnt++;
        $l->stop;
    };
}

sub check_appears {
    my $name = shift;
    return sub {
        my ($h, $prev, $curr, $err) = @_;
        ok(!$err, "fspoll callback without error ($name)");
        $prev->[STAT_TYPE] = $curr->[STAT_TYPE] = 0;
        my $prev_sum = 0;
        $prev_sum += $_ for @$prev;
        my $curr_sum = 0;
        $curr_sum += $_ for @$curr;
        is($prev_sum, 0, "fspoll callback prev is empty ($name)");
        cmp_ok($curr_sum, '>', 0, "fspoll callback curr is not empty ($name)");
        $call_cnt++;
        $l->stop;
    };
}

sub check_changes {
    my ($fields, $name) = @_;
    $fields = [$fields] unless ref $fields;
    my %left = map {$_ => 1} @$fields;
    return sub {
        my ($h, $prev, $curr, $err) = @_;
        ok(!$err, "fspoll callback without error ($name)");
        foreach my $field (@$fields) {
            next if $prev->[$field] == $curr->[$field];
            delete $left{$field};
        }
        unless (%left) {
            pass("required fields changed");
            $call_cnt++;
            $l->stop;
        }
    };
}

sub check_call_cnt {
    my $cnt = shift;
    is $call_cnt, $cnt, "call cnt";
    $call_cnt = 0;
}
