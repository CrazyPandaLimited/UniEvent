use 5.012;
use warnings;

use UniEvent;
use CPP::catch;
use XS::Loader;

XS::Loader::load_tests();

$ENV{"UNIEVENT_TEST_SSL"} = 1;

catch_run("[panda-event][ssl]");
