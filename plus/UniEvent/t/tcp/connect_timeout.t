use 5.012;
use lib 't/lib';
use MyTest;
use Test::Catch;

pass("off");
done_testing();
exit;

alarm(100);

variate(qw/ssl socks/, sub {
    catch_run('[tcp-connect-timeout]');
});

done_testing();
 
