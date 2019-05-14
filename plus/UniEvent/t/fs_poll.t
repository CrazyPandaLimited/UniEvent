use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Error;

my ($l, $fsp, $t, $err);
my @stat_fields = qw/dev ino mode nlink uid gid rdev size atime mtime ctime blksize blocks flags gen birthtime/;

$l = UniEvent::Loop->default_loop;
$fsp = new UniEvent::FsPoll;
$t = new UniEvent::Timer;

############ FILE TRACKING

# non-existant file
$fsp->start(var 'file', 0.01);
$fsp->fs_poll_callback(cb(ENOENT, undef, "watch non-existant file"));
# path getter
is($fsp->path, var 'file', "path getter works");

$l->run;

# check that callback with not-exists error called only once (until something changes)
$t->event->add(sub {$l->stop});
time_mark();
$t->start(0.01);
$l->run;
check_mark(0.01, "no more error callbacks until something changes");
$t->stop;
$t->start(0.001);
$t->event->remove_all;

# file appears
$fsp->fs_poll_callback(cb(undef, 'all', "file appears"));
create_file('file')->();
$l->run;

# file mtime
$fsp->fs_poll_callback(cb(undef, 'mtime', "file mtime"));
$t->event->remove_all;
$t->event->add(change_file_mtime('file'));
$l->run;

# stat as hash
#ok(!$fsp->stat_as_hash, "default stat is array");
#$fsp->stat_as_hash(1);
#ok($fsp->stat_as_hash, "now is as hash");
$fsp->fs_poll_callback(cb(undef, 'mtime', "file mtime", 1));
$t->event->remove_all;
$t->event->add(change_file_mtime('file'));
$l->run;
#$fsp->stat_as_hash(0);

# file contents
$fsp->fs_poll_callback(cb(undef, 'size', "file content"));
$t->event->remove_all;
$t->event->add(change_file('file'));
$l->run;

# stop
$t->stop;
$fsp->stop;
$l->run;
pass("stop works");



sub cb {
    my ($err_code, $fields_must_change, $test_name) = @_;
    $test_name ||= '';
    return sub {
        my ($h, $prev, $curr, $err) = @_;
        if ($err_code) {
            isa_ok($err, 'UniEvent::CodeError', "fspoll callback error correct ($test_name)");
            is($err->code, $err_code, "fspoll callback error code correct ($test_name)") if $err;
        } else {
            ok(!$err, "fspoll callback without error ($test_name)");
            ok($prev && $curr && ref($prev) && ref($curr), "fspoll callback stat structs present without error ($test_name)");
            if ($fields_must_change) {
                for ($prev, $curr) { $_ = stat_av2hv($_) }
                if ($fields_must_change eq 'all') {
                    my $prev_sum = 0;
                    $prev_sum += $_ for values %$prev;
                    my $curr_sum = 0;
                    $curr_sum += $_ for values %$curr;
                    is($prev_sum, 0, "fspoll callback prev is empty ($test_name)");
                    cmp_ok($curr_sum, '>', 0, "fspoll callback curr is not empty ($test_name)");
                } else {
                    $fields_must_change = [$fields_must_change] unless ref $fields_must_change;
                    isnt($prev->{$_}, $curr->{$_}, "fspoll callback stat field '$_' changed ($test_name)") for @$fields_must_change;
                }
            }
        }
        $l->stop;
    };
}

sub stat_av2hv {
    my $av = shift;
    my $hv = {};
    $hv->{$_} = shift @$av for @stat_fields;
    return $hv;
}







__END__


use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Error;

my @stat_fields = qw/dev ino mode nlink uid gid rdev size atime mtime ctime blksize blocks flags gen birthtime/;

my $call_cnt;
my $l = UniEvent::Loop->default_loop;

#subtest 'non-existant file' => sub {
#    my $h = new UniEvent::FsPoll;
#    $h->start(var 'file', 0.01);
#    $h->fs_poll_callback(cb(ENOENT, undef, "watch non-existant file"));
#    is($h->path, var 'file', "path getter works");
#    $l->run;
#    $l->run_nowait; # must be called only once
#    check_call_cnt(1);
#};
#
#subtest 'file appears' => sub {
#    my $h = new UniEvent::FsPoll;
#    $h->start(var 'file', 0.01);
#    $h->fs_poll_callback(cb(ENOENT, undef, "watch non-existant file"));
#    $l->run;
#    check_call_cnt(1);
#    $h->fs_poll_callback(cb(undef, 'all', "file appears"));
#    create_file('file')->();
#    $l->run;
#    check_call_cnt(1);
#    unlink_file('file');
#};
#
#subtest 'mtime' => sub {
#    create_file('file')->();
#    my $h = new UniEvent::FsPoll;
#    $h->start(var 'file', 0.01);
#    $h->fs_poll_callback(cb(undef, 'mtime', "file mtime"));
#    my $t = UE::Timer->once(0.01, change_file_mtime('file'));
#    $l->run;
#    check_call_cnt(1);
#    unlink_file('file');
#};
#
#subtest 'file contents' => sub {
#    create_file('file')->();
#    my $h = new UniEvent::FsPoll;
#    $h->start(var 'file', 0.01);
#    $h->fs_poll_callback(cb(undef, 'size', "file content"));
#    my $t = UE::Timer->once(0.01, change_file('file'));
#    $l->run;
#    check_call_cnt(1);
#    unlink_file('file');
#};

subtest 'stop' => sub {
    my $l = UE::Loop->new;
    create_file('file')->();
    my $h = new UniEvent::FsPoll($l);
    $h->start(var 'file', 0.01);
    #$h->stop;
    $h->fs_poll_callback(sub {
        say "changed";
    });
    my $t = UE::Timer->once(1, sub {
        say "timer";
        $h->stop;
    }, $l);
    $l->run;
#    say "1";
#    my $t = UE::Timer->once(0.01, sub { say "stopping" ; $h->stop });
#    #$l->run_nowait;
#    say "2";
#    #$h->stop;
#    say "3";
#    $l->run;
#    say "4";
#    $l->run;
#    say "5";
#    check_call_cnt(0);
    unlink_file('file');
};

## stop
#$t->stop;
#$fsp->stop;
#$l->run;
#pass("stop works");
#
## reset
#$t->start(0.001);
#$t->timer_callback(change_file_mtime('file'));
#$fsp->start(var 'file', 0.01);
#$fsp->fs_poll_callback(cb(undef, ['mtime'], "file content"));
#$l->run;
#$t->stop;
#$fsp->reset;
#$l->run;
#pass("reset works");
#
## file remove
#$t->start(0.001);
#$fsp->start(var 'file', 0.01);
#$fsp->fs_poll_callback(cb(ENOENT, undef, "file remove"));
#$t->timer_callback(unlink_file('file'));
#$l->run;

done_testing();


sub cb {
    my ($err_code, $fields_must_change, $test_name) = @_;
    $test_name ||= '';
    return sub {
        my ($h, $prev, $curr, $err) = @_;
        $call_cnt++;
        if ($err_code) {
            is($err->code, $err_code, "fspoll callback error code correct ($test_name)") if $err;
        } else {
            ok(!$err, "fspoll callback without error ($test_name)");
            ok($prev && $curr && ref($prev) && ref($curr), "fspoll callback stat structs present without error ($test_name)");
            if ($fields_must_change) {
                for ($prev, $curr) { $_ = stat_av2hv($_) }
                if ($fields_must_change eq 'all') {
                    my $prev_sum = 0;
                    $prev_sum += $_ for values %$prev;
                    my $curr_sum = 0;
                    $curr_sum += $_ for values %$curr;
                    is($prev_sum, 0, "fspoll callback prev is empty ($test_name)");
                    cmp_ok($curr_sum, '>', 0, "fspoll callback curr is not empty ($test_name)");
                } else {
                    $fields_must_change = [$fields_must_change] unless ref $fields_must_change;
                    isnt($prev->{$_}, $curr->{$_}, "fspoll callback stat field '$_' changed ($test_name)") for @$fields_must_change;
                }
            }
        }
        $l->stop;
    };
}

sub check_call_cnt {
    my $cnt = shift;
    is $call_cnt, $cnt, "call cnt";
    $call_cnt = 0;
}

sub stat_av2hv {
    my $av = shift;
    my $hv = {};
    $hv->{$_} = shift @$av for @stat_fields;
    return $hv;
}