package MyTest;
use 5.012;
use warnings;
use UniEvent;
use Test::More;
use Test::Deep;
use Test2::IPC;
use Test::Catch;
use Test::Exception;

XS::Loader::load('MyTest');

$SIG{PIPE} = 'IGNORE';

my $rdir = "t/var/$$";
my $have_time_hires = eval "require Time::HiRes; 1;";
my $last_time_mark;
my %used_mtimes;

init();

sub init {
    if ($ENV{LOGGER}) {
        require XLog;
        XLog::set_logger(sub { say $_[1] });
        XLog::set_level(XLog::VERBOSE_DEBUG());
        XLog::set_level(XLog::INFO(), "UniEvent::SSL");
    }    
    
    # for file tests
    UniEvent::Fs::remove_all($rdir) if -d $rdir;
    UniEvent::Fs::mkpath($rdir);
    
    # if something goes wrong, loop hangs. Make tests fail with SIGALRM instead of hanging forever.
    # each test must not last longer than 10 seconds. If needed, set alarm(more_than_10s) in your test
    alarm(15) unless defined $DB::header;
}

sub import {
    my ($class) = @_;

    my $caller = caller();
    foreach my $sym_name (qw/
        linux freebsd win32 darwin winWSL netbsd openbsd dragonfly
        is cmp_deeply ok done_testing skip isnt time_mark check_mark pass fail cmp_ok like isa_ok unlike diag plan
        var pipe create_file create_dir move change_file_mtime change_file unlink_file remove_dir subtest new_ok dies_ok catch_run any
    /) {
        no strict 'refs';
        *{"${caller}::$sym_name"} = \&{$sym_name};
    }
}

sub linux     { $^O eq 'linux' }
sub freebsd   { $^O eq 'freebsd' }
sub win32     { $^O eq 'MSWin32' }
sub darwin    { $^O eq 'darwin' }
sub netbsd    { $^O eq 'netbsd' }
sub openbsd   { $^O eq 'openbsd' }
sub dragonfly { $^O eq 'dragonfly' }
sub winWSL    { linux() && `egrep "(Microsoft|WSL)" /proc/version` }

sub time_mark {
    return unless $have_time_hires;
    $last_time_mark = Time::HiRes::time();
}

sub check_mark {
    return unless $have_time_hires;
    my ($approx, $msg) = @_;
    my $delta = Time::HiRes::time() - $last_time_mark;
    cmp_ok($delta, '>=', $approx*0.8, $msg);
}

sub var ($) { return "$rdir/$_[0]" }

sub pipe ($) {
    if (win32()) {
        return "\\\\.\\pipe\\$_[0]";
    } else {
        return var "pipe_$_[0]";
    }
}

END { # clean up after file tests
    UniEvent::Fs::remove_all($rdir) if -d $rdir;
}

1;
