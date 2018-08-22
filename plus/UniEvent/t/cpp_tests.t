use 5.012;
use warnings;

use Test::More;
use UniEvent;
use Benchmark qw/timethese/;

plan skip_all => 'rebuild Makefile.PL adding TEST_FULL=1 to enable all tests' unless UniEvent->can('_run_cpp_tests_');

ok (UniEvent::_run_cpp_tests_(''));

if(UniEvent->can('_benchmark_regular_resolver')) {
    timethese(-2, {
        'regular_resolver' => sub { UniEvent::_benchmark_regular_resolver(); },
        'cached_resolver' => sub { UniEvent::_benchmark_cached_resolver(); }
    });
}

done_testing();
