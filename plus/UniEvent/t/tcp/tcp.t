use 5.012;
use lib 't/lib';
use MyTest;
use Test::Catch;

alarm(10000);
catch_run('[tcp]');
#variate_catch('[tcp]', qw/ssl buf/);

done_testing();
