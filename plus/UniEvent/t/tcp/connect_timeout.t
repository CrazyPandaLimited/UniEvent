use 5.012;
use lib 't/lib';
use MyTest;
use Test::Catch;

variate_catch('[tcp-connect-timeout]', qw/ssl socks/);

done_testing();
 
