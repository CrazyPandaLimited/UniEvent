use 5.012;
use warnings;
use lib 't/lib'; use MyTest;
use UniEvent::Error;

# check constants existance
cmp_ok(
  ERRNO_E2BIG + ERRNO_EACCES + ERRNO_EADDRINUSE + ERRNO_EADDRNOTAVAIL + ERRNO_EAFNOSUPPORT + ERRNO_EAGAIN + ERRNO_EAI_ADDRFAMILY +
  ERRNO_EAI_AGAIN + ERRNO_EAI_BADFLAGS + ERRNO_EAI_BADHINTS + ERRNO_EAI_CANCELED + ERRNO_EAI_FAIL + ERRNO_EAI_FAMILY + ERRNO_EAI_MEMORY +
  ERRNO_EAI_NODATA + ERRNO_EAI_NONAME + ERRNO_EAI_OVERFLOW + ERRNO_EAI_PROTOCOL + ERRNO_EAI_SERVICE + ERRNO_EAI_SOCKTYPE +
  ERRNO_EALREADY + ERRNO_EBADF + ERRNO_EBUSY + ERRNO_ECANCELED + ERRNO_ECHARSET + ERRNO_ECONNABORTED + ERRNO_ECONNREFUSED +
  ERRNO_ECONNRESET + ERRNO_EDESTADDRREQ + ERRNO_EEXIST + ERRNO_EFAULT + ERRNO_EFBIG + ERRNO_EHOSTUNREACH + ERRNO_EINTR + ERRNO_EINVAL +
  ERRNO_EIO + ERRNO_EISCONN + ERRNO_EISDIR + ERRNO_ELOOP + ERRNO_EMFILE + ERRNO_EMSGSIZE + ERRNO_ENAMETOOLONG + ERRNO_ENETDOWN +
  ERRNO_ENETUNREACH + ERRNO_ENFILE + ERRNO_ENOBUFS + ERRNO_ENODEV + ERRNO_ENOENT + ERRNO_ENOMEM + ERRNO_ENONET + ERRNO_ENOPROTOOPT +
  ERRNO_ENOSPC + ERRNO_ENOSYS + ERRNO_ENOTCONN + ERRNO_ENOTDIR + ERRNO_ENOTEMPTY + ERRNO_ENOTSOCK + ERRNO_ENOTSUP + ERRNO_EPERM +
  ERRNO_EPIPE + ERRNO_EPROTO + ERRNO_EPROTONOSUPPORT + ERRNO_EPROTOTYPE + ERRNO_ERANGE + ERRNO_EROFS + ERRNO_ESHUTDOWN + ERRNO_ESPIPE +
  ERRNO_ESRCH + ERRNO_ETIMEDOUT + ERRNO_ETXTBSY + ERRNO_EXDEV + ERRNO_UNKNOWN + ERRNO_EOF + ERRNO_ENXIO + ERRNO_EMLINK + ERRNO_SSL,
  '<', 0, "constants exist",
);

subtest "Error" => sub {
    my $e = new_ok "UniEvent::Error" => ["message"];
    is $e->what, "message";
    is $e, $e->what;
    my $c = $e->clone;
    isa_ok $c, ref($e);
    is $c, $e, "clone ok";
};

subtest "ImplRequiredError" => sub {
    my $e = new_ok "UniEvent::ImplRequiredError" => ["message"];
    like $e->what, qr/message/;
    my $c = $e->clone;
    isa_ok $c, ref($e);
    is $c, $e, "clone ok";
};

subtest "CodeError" => sub {
    my $e = new_ok "UniEvent::CodeError" => [ERRNO_EACCES];
    is $e->code, ERRNO_EACCES, "code ok";
    ok $e->what, "what present";
    is $e->name, "EACCES", "name ok";
    ok $e->str, "str present";
    my $c = $e->clone;
    isa_ok $c, ref($e);
    is $c, $e, "clone ok";
};

subtest "SSLError" => sub {
    my $ssl_code = 1;
    my $e = new_ok "UniEvent::SSLError" => [$ssl_code];
    is $e->code, ERRNO_SSL, "code ok";
    ok $e->what, "what present";
    is $e->name, "SSL", "name ok";
    ok $e->str, "str present";
    is $e->ssl_code, $ssl_code, "ssl code";
    $e->openssl_code;
    $e->library;
    $e->function;
    $e->reason;
    $e->library_str;
    $e->function_str;
    $e->reason_str;
    $e->str;
    my $c = $e->clone;
    isa_ok $c, ref($e);
    is $c, $e, "clone ok";
};

done_testing();
