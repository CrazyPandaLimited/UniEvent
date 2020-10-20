use 5.012;
use warnings;
use lib 't/lib';
use MyTest;
use lib 't/streamer';
use TestStreamer;


subtest 'file in, stream out' => sub {
    my $test = UE::Test::Async->new(1);
    my $p = MyTest::make_p2p($test->loop);
    my $f = 'src/xs/unievent.cc';
    my $z = 'ku-ku';
    my $o = UE::Streamer::StreamOutput->new($p->{sconn});
    my $i = UE::Streamer::FileInput->new($f, 10000);
    my $s = UE::Streamer->new($i, $o, 100000, $test->loop);
    $s->start();    my $count = 0;
    $p->{client}->read_callback(sub {
        die $_[2] if $_[2];
        say "got piece: ", length($_[1]);
        $count += length($_[1]);
    });    $s->finish_callback(sub {
        my $err = shift;
        is $err, undef;
        $test->happens();
        $test->loop->stop();
    });
    $p->{sconn}->write($z);
    $p->{sconn}->write($z);
    $test->run();
    my $sz = (stat($f))[7];
    is $count, $sz + length($z) * 2, "got required";
    say "end of test";
};

done_testing;
