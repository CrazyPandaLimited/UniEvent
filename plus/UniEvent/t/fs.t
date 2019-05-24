use 5.012;
use lib 't/lib';
use MyTest;
use UniEvent::Fs;
use UniEvent::Error;

catch_run('[fs]');

BEGIN { *Fs:: = *UniEvent::Fs:: }

my $vdir = var("");
my $file = var("file");
my $dir  = var("dir");

sub subtest ($&) {
    my ($name, $sub) = @_;
    MyTest::subtest($name, $sub);
}

subtest 'constants' => sub {
    foreach my $name (qw/
        DEFAULT_FILE_MODE
        DEFAULT_DIR_MODE
        
        OPEN_APPEND
        OPEN_CREAT
        OPEN_DIRECT
        OPEN_DIRECTORY
        OPEN_DSYNC
        OPEN_EXCL
        OPEN_EXLOCK
        OPEN_NOATIME
        OPEN_NOCTTY
        OPEN_NOFOLLOW
        OPEN_NONBLOCK
        OPEN_RANDOM
        OPEN_RDONLY
        OPEN_RDWR
        OPEN_SEQUENTIAL
        OPEN_SHORT_LIVED
        OPEN_SYMLINK
        OPEN_SYNC
        OPEN_TEMPORARY
        OPEN_TRUNC
        OPEN_WRONLY

        SYMLINK_DIR
        SYMLINK_JUNCTION

        COPYFILE_EXCL
        COPYFILE_FICLONE
        COPYFILE_FICLONE_FORCE

        FTYPE_BLOCK
        FTYPE_CHAR
        FTYPE_DIR
        FTYPE_FIFO
        FTYPE_LINK
        FTYPE_FILE
        FTYPE_SOCKET
        FTYPE_UNKNOWN
    /) {
        ok(defined eval("$name"), "$name");
    }
};

subtest 'xs-sync' => sub {
    subtest 'mkdir' => sub {
        subtest 'non-existant' => sub {
            ok Fs::mkdir($dir);
            ok Fs::isdir($dir);
        };
        subtest 'dir exists' => sub {
            Fs::mkdir($dir);
            my ($ok, $err) = Fs::mkdir($dir);
            ok !$ok;
            is $err->code, EEXIST;
        };
    };
};