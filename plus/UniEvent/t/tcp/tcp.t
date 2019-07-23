use 5.012;
use lib 't/lib';
use MyTest;
use Test::Catch;

variate_catch('[tcp]', qw/ssl buf/);

done_testing();
