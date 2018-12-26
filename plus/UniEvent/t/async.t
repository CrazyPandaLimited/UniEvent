use 5.012;
use lib 't/lib';
use MyTest;
BEGIN { plan skip_all => 'disabled'; }
use Test::Catch '[async]';