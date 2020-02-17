use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Error;

subtest "error constants" => sub {
    foreach my $row (
        ["address_family_not_supported", "EAFNOSUPPORT"],
        ["address_in_use", "EADDRINUSE"],
        ["address_not_available", "EADDRNOTAVAIL"],
        ["already_connected", "EISCONN"],
        ["argument_list_too_long", "E2BIG"],
        ["argument_out_of_domain", "EDOM"],
        ["bad_address", "EFAULT"],
        ["bad_file_descriptor", "EBADF"],
        ["broken_pipe", "EPIPE"],
        ["connection_aborted", "ECONNABORTED"],
        ["connection_already_in_progress", "EALREADY"],
        ["connection_refused", "ECONNREFUSED"],
        ["connection_reset", "ECONNRESET"],
        ["cross_device_link", "EXDEV"],
        ["destination_address_required", "EDESTADDRREQ"],
        ["device_or_resource_busy", "EBUSY"],
        ["directory_not_empty", "ENOTEMPTY"],
        ["executable_format_error", "ENOEXEC"],
        ["file_exists", "EEXIST"],
        ["file_too_large", "EFBIG"],
        ["filename_too_long", "ENAMETOOLONG"],
        ["function_not_supported", "ENOSYS"],
        ["host_unreachable", "EHOSTUNREACH"],
        ["illegal_byte_sequence", "EILSEQ"],
        ["inappropriate_io_control_operation", "ENOTTY"],
        ["interrupted", "EINTR"],
        ["invalid_argument", "EINVAL"],
        ["invalid_seek", "ESPIPE"],
        ["io_error", "EIO"],
        ["is_a_directory", "EISDIR"],
        ["message_size", "EMSGSIZE"],
        ["network_down", "ENETDOWN"],
        ["network_reset", "ENETRESET"],
        ["network_unreachable", "ENETUNREACH"],
        ["no_buffer_space", "ENOBUFS"],
        ["no_child_process", "ECHILD"],
        ["no_lock_available", "ENOLCK"],
        ["no_protocol_option", "ENOPROTOOPT"],
        ["no_space_on_device", "ENOSPC"],
        ["no_such_device_or_address", "ENXIO"],
        ["no_such_device", "ENODEV"],
        ["no_such_file_or_directory", "ENOENT"],
        ["no_such_process", "ESRCH"],
        ["not_a_directory", "ENOTDIR"],
        ["not_a_socket", "ENOTSOCK"],
        ["not_connected", "ENOTCONN"],
        ["not_enough_memory", "ENOMEM"],
        ["not_supported", "ENOTSUP"],
        ["operation_canceled", "ECANCELED"],
        ["operation_in_progress", "EINPROGRESS"],
        ["operation_not_permitted", "EPERM"],
        ["operation_not_supported", "EOPNOTSUPP"],
        ["operation_would_block", "EWOULDBLOCK"],
        ["permission_denied", "EACCES"],
        ["protocol_error", "EPROTO"],
        ["protocol_not_supported", "EPROTONOSUPPORT"],
        ["read_only_file_system", "EROFS"],
        ["resource_deadlock_would_occur", "EDEADLK"],
        ["resource_unavailable_try_again", "EAGAIN"],
        ["result_out_of_range", "ERANGE"],
        ["timed_out", "ETIMEDOUT"],
        ["too_many_files_open_in_system", "ENFILE"],
        ["too_many_files_open", "EMFILE"],
        ["too_many_links", "EMLINK"],
        ["too_many_symbolic_link_levels", "ELOOP"],
        ["value_too_large", "EOVERFLOW"],
        ["wrong_protocol_type", "EPROTOTYPE"],
        ###########################################################
        ["ssl_error", "ESSL"],
        ["socks_error", "ESOCKS"],
        ["resolve_error", "ERESOLVE"],
        ["ai_address_family_not_supported", "EAI_ADDRFAMILY"],
        ["ai_temporary_failure", "EAI_AGAIN"],
        ["ai_bad_flags", "EAI_BADFLAGS"],
        ["ai_bad_hints", "EAI_BADHINTS"],
        ["ai_request_canceled", "EAI_CANCELED"],
        ["ai_permanent_failure", "EAI_FAIL"],
        ["ai_family_not_supported", "EAI_FAMILY"],
        ["ai_out_of_memory", "EAI_MEMORY"],
        ["ai_no_address", "EAI_NODATA"],
        ["ai_unknown_node_or_service", "EAI_NONAME"],
        ["ai_argument_buffer_overflow", "EAI_OVERFLOW"],
        ["ai_resolved_protocol_unknown", "EAI_PROTOCOL"],
        ["ai_service_not_available_for_socket_type", "EAI_SERVICE"],
        ["ai_socket_type_not_supported", "EAI_SOCKTYPE"],
        ["invalid_unicode_character", "ECHARSET"],
        ["not_on_network", "ENONET"],
        ["transport_endpoint_shutdown", "ESHUTDOWN"],
        ["unknown_error", "UNKNOWN"],
        ["host_down", "EHOSTDOWN"],
        ["remote_io", "EREMOTEIO"],
    ) {
        my ($long, $short) = @$row;
        my $long_sub = UniEvent::Error->can($long);
        my $val = $long_sub->()->value;
        cmp_ok($val, '>', 0, "UniEvent::Error::$long(): $val");
        my $short_sub = UniEvent::Error->can($short);
        is($short_sub->(), $val, "UniEvent::Error::$short()");
        is(eval("$short"), $val, "$short exported");
    }
};

subtest "Error" => sub {
    my $e = new_ok "UniEvent::Error" => ["message"];
    is $e->what, "message";
    is $e, $e->what;
    my $c = $e->clone;
    isa_ok $c, ref($e);
    is $c, $e, "clone ok";
};

subtest "CodeError" => sub {
    for my $row ([EACCES->value, UniEvent::CodeError::generic_category()], [ESSL->value, UniEvent::CodeError::unievent_category()]) {
        my $e = new_ok "UniEvent::CodeError" => $row;
        is $e->code, $row->[0], "code ok";
        is $e->category, $row->[1], "category ok";
        ok $e->descr, "descr present";
        ok $e->what, "what present";
        my $c = $e->clone;
        isa_ok $c, ref($e);
        is $c, $e, "clone ok";
    }
};

subtest "SSLError" => sub {
    my $ssl_code = 1;
    my $e = new_ok "UniEvent::SSLError" => [$ssl_code];
    is $e->code, ESSL, "code ok";
    ok $e->what, "what present";
    ok $e->descr, "descr present";
    is $e->ssl_code, $ssl_code, "ssl code";
    $e->openssl_code;
    $e->library;
    $e->function;
    $e->reason;
    $e->library_str;
    $e->function_str;
    $e->reason_str;
    $e->descr;
    my $c = $e->clone;
    isa_ok $c, ref($e);
    is $c, $e, "clone ok";
};

done_testing();
