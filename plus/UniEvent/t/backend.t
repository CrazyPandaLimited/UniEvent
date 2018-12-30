use 5.012;
use warnings;
use lib 't/lib'; use MyTest;

subtest "uv backend" => sub {
    ok UniEvent::Backend::UV(), "exists";
    is UniEvent::Backend::UV()->name, "uv", "name ok";
};

subtest "uv is a default default_backend" => sub {
    ok UniEvent::default_backend(), "present";
    is UniEvent::default_backend()->name, "uv", "is uv";
};

dies_ok { UniEvent::set_default_backend(undef) } "cannot set null as default backend";

subtest "change backend before actions" => sub {
    UniEvent::set_default_backend(UniEvent::Backend::UV());
    is UniEvent::default_backend()->name, "uv";
};

UniEvent::Loop->new(UniEvent::Backend::UV()); # create local loop

subtest "change backend before global/default loop created" => sub {
    UniEvent::set_default_backend(UniEvent::Backend::UV());
    is UniEvent::default_backend()->name, "uv";
};

ok(UniEvent::Loop->global_loop, "global loop created");
ok(UniEvent::Loop->default_loop, "default loop created");

dies_ok { UniEvent::set_default_backend(UniEvent::Backend::UV()) } "cannot change backend after global/default loop created";

done_testing();
