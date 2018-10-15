package MyTest;
use 5.012;
use warnings;
use UniEvent;
use Test::More;
use Test::Deep;
use Test::Exception;
use File::Path qw/make_path remove_tree/;

XS::Loader::load_tests('MyTest');

my $rdir = "t/var/$$";
my $have_time_hires = eval "require Time::HiRes; 1;";
my $last_time_mark;
my %used_mtimes;

init();

sub init {
    remove_tree($rdir, {error => \my $err} );
    # if something goes wrong, loop hangs. Make tests fail with SIGALRM instead of hanging forever.
    # each test must not last longer than 10 seconds. If needed, set alarm(more_than_10s) in your test
    alarm(10) unless defined $DB::header;

    make_path($rdir, {mode => 0755});
}

sub import {
    my ($class) = @_;

    my $caller = caller();
    foreach my $sym_name (qw/
        is cmp_deeply ok done_testing skip isnt time_mark check_mark pass fail cmp_ok like isa_ok unlike diag plan
        var create_file create_dir move change_file_mtime change_file unlink_file remove_dir subtest new_ok dies_ok
    /) {
        no strict 'refs';
        *{"${caller}::$sym_name"} = *$sym_name;
    }
}

sub time_mark {
    return unless $have_time_hires;
    $last_time_mark = Time::HiRes::time();
}

sub check_mark {
    return unless $have_time_hires;
    my ($approx, $msg) = @_;
    my $min = $approx*0.8;
    my $max = $approx ? $approx*1.3 : 0.001;
    my $delta = Time::HiRes::time() - $last_time_mark;
    cmp_ok($delta, '>=', $min, $msg) if defined $min;
    cmp_ok($delta, '<=', $max, $msg) if defined $max;
}

sub var ($) { return "$rdir/$_[0]" }

sub create_file {
    my $path = "$rdir/".shift();
    return sub {
        open my $fh, '>', $path or die "$path: $!";
        close $fh;
    };
}

sub create_dir {
    my $path = "$rdir/".shift();
    return sub {
        make_path($path, {mode => 0755});
    };
}

sub move {
    my $old = "$rdir/".shift();
    my $new = "$rdir/".shift();
    return sub {
        rename $old, $new or die "rename $old -> $new: $!";
    };
}

sub change_file_mtime {
    my $path = "$rdir/".shift();
    return sub {
        my $time = time();
        my $rnd;
        do { $rnd = int rand 10000000 } while $used_mtimes{$rnd}++;
        utime($time-$rnd, $time-$rnd, $path) or die "$path: $!";
    };
}

sub change_file {
    my $path = "$rdir/".shift();
    return sub {
        open my $fh, '>>', $path or die "$path: $!";
        print $fh "content".int(rand 2**32)."\n";
        close $fh;
    };
}

sub unlink_file {
    my $path = "$rdir/".shift();
    return sub {
        unlink($path);
    };
}

sub remove_dir {
    my $path = "$rdir/".shift();
    return sub {
        remove_tree($path);
    };
}

END {
    remove_tree($rdir);
}

1;
