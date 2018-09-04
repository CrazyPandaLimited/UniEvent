use 5.012;
use warnings;
use lib 't'; use PETest;
use UniEvent::Error;
use UniEvent::FSEvent;

# TODO: check WATCH_ENTRY / STAT / RECURSIVE flags behaviour when they become working in libuv

my ($l, $fse, $t, $err);

# check constants
cmp_ok(WATCH_ENTRY + STAT + RECURSIVE + RENAME + CHANGE, '>', 0, "constants exist");

$l = UniEvent::Loop->default_loop;
$fse = new UniEvent::FSEvent;
$t = new UniEvent::Timer;
$t->start(0.001);

########### FILE TRACKING

# FSEvent doesn't track non-existant files
dies_ok { $fse->start(var 'file') } "exception when tries to handle non-existant file";
$err = $@;
isa_ok($err, 'UniEvent::CodeError');
is($err->code, ERRNO_ENOENT, "error code is correct");

create_file('file')->();

$fse->start(var 'file');
# path getter
is($fse->path, var 'file', "path getter works");

# file mtime
$fse->fs_event_callback(cb(CHANGE, 'file', "file mtime"));
$t->timer_callback(change_file_mtime('file'));
$l->run;

# file contents
$fse->fs_event_callback(cb(CHANGE, 'file', "file content"));
$t->timer_callback(change_file('file'));
$l->run;

# file rename
$fse->fs_event_callback(cb(RENAME, 'file', "file rename"));
$t->timer_callback(move('file', 'file2'));
$l->run;
is($fse->path, var 'file', "path getter still return old path");

# still keep tracking file after rename
$fse->fs_event_callback(cb(CHANGE, 'file', "renamed file content"));
$t->timer_callback(change_file('file2'));
$l->run;

# stop
$t->stop;
$fse->stop;
$l->run;
pass("stop works");

# reset
$fse->start(var 'file2');
$fse->fs_event_callback(cb(CHANGE, 'file2'));
$t->start(0.001);
$l->run;
$t->stop;
$fse->reset;
$l->run;
pass("reset works");

# file remove - libuv somewhy calls callback 3 times, first with status CHANGE, then 2 times with status RENAME
$t->start(0.001);
$fse->start(var 'file2');
$fse->fs_event_callback(cb(undef, 'file2', "renamed file remove"));
$t->timer_callback(unlink_file('file2'));
$l->run;

########### DIR TRACKING

# FSEvent doesn't track non-existant dirs
$fse->stop;
ok(!eval {$fse->start(var 'dir'); 1}, "exception when tries to handle non-existant dir");
$fse->stop;

create_dir('dir')->();
$fse->start(var 'dir');
$t->start(0.001);

# path getter
is($fse->path, var 'dir', "path getter works");

# dir mtime
$fse->fs_event_callback(cb(CHANGE+RENAME, 'dir', "dir mtime"));
$t->timer_callback(change_file_mtime('dir'));
$l->run;

# dir contents
$fse->fs_event_callback(cb(RENAME, 'ifile', "dir content: add file"));
$t->timer_callback(create_file('dir/ifile'));
$l->run;
$fse->fs_event_callback(cb(RENAME, "ifile|ifile2", "dir content: rename file")); # 2 callbacks - for old remove and new create
$t->timer_callback(move('dir/ifile', 'dir/ifile2'));
$l->run;
$fse->fs_event_callback(cb(RENAME, 'ifile2', "dir content: remove file"));
$t->timer_callback(unlink_file('dir/ifile2'));
$l->run;

# dir rename
$fse->fs_event_callback(cb(RENAME, 'dir', "dir rename"));
$t->timer_callback(move('dir', 'dir2'));
$l->run;
is($fse->path, var 'dir', "path getter still return old path");

# still keep tracking dir after rename
$fse->fs_event_callback(cb(CHANGE+RENAME, 'dir', "renamed dir mtime"));
$t->timer_callback(change_file_mtime('dir2'));
$l->run;
$fse->fs_event_callback(cb(RENAME, 'ifile', "renamed dir content: add file"));
$t->timer_callback(create_file('dir2/ifile'));
$l->run;
$fse->fs_event_callback(cb(RENAME, 'ifile', "renamed dir content: remove file"));
$t->timer_callback(unlink_file('dir2/ifile'));
$l->run;

# dir remove - libuv somewhy calls callback 2 times
$fse->fs_event_callback(cb(RENAME, 'dir', "renamed dir remove"));
$t->timer_callback(remove_dir('dir2'));
$l->run;


done_testing();


sub cb {
    my ($check_event, $check_filename, $test_name) = @_;
    $test_name ||= '';
    return sub {
        my ($h, $filename, $events) = @_;
        is($events, $check_event, "fs callback event    is correct ($test_name)") if defined $check_event;
        like($filename, qr/^$check_filename$/, "fs callback filename is correct ($test_name)");
        $l->stop;
    };
}