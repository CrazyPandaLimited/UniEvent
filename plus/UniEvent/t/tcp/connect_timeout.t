use 5.012;
use lib 't/lib';
use MyTest;
use Test::Catch;
plan skip_all => 'disabled';

variate_catch('[tcp-connect-timeout]', qw/ssl/);

done_testing();
 
