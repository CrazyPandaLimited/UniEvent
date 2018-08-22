use 5.012;
use warnings;
use lib 't'; use PETest;
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
  ERRNO_ESRCH + ERRNO_ETIMEDOUT + ERRNO_ETXTBSY + ERRNO_EXDEV + ERRNO_UNKNOWN + ERRNO_EOF + ERRNO_ENXIO + ERRNO_EMLINK,
  '<', 0, "constants exist",
);

my ($e);

$e = new UniEvent::Error({what => "what", mess => "mess"});
is($e->what, "what");
like($e->to_string, qr/what/);
like($e->to_string, qr/mess/);
like($e->to_string, qr/Error/);
is($e.'', $e->to_string);

isa_ok('UniEvent::RequestError', 'UniEvent::Error');
isa_ok('UniEvent::CodeError', 'UniEvent::Error');

$e = new UniEvent::CodeError({what => 'what123', mess => 'mess', code => 777, name => 'name', str => 'str'});
is($e->what, 'what123');
is($e->code, 777);
is($e->name, 'name');
is($e->str, 'str');
is($e.'', $e->to_string);
like($e.'', qr/what123/);
like($e.'', qr/mess/);
unlike($e.'', qr/777/);
unlike($e.'', qr/name/);
like($e.'', qr/CodeError/);

isa_ok('UniEvent::LoopError',      'UniEvent::CodeError');
isa_ok('UniEvent::HandleError',    'UniEvent::CodeError');
isa_ok('UniEvent::OperationError', 'UniEvent::CodeError');
isa_ok('UniEvent::AsyncError',     'UniEvent::HandleError');
isa_ok('UniEvent::CheckError',     'UniEvent::HandleError');
isa_ok('UniEvent::FSEventError',   'UniEvent::HandleError');
isa_ok('UniEvent::FSPollError',    'UniEvent::HandleError');
isa_ok('UniEvent::FSRequestError', 'UniEvent::HandleError');
isa_ok('UniEvent::PollError',      'UniEvent::HandleError');
isa_ok('UniEvent::ProcessError',   'UniEvent::HandleError');
isa_ok('UniEvent::SignalError',    'UniEvent::HandleError');
isa_ok('UniEvent::TimerError',     'UniEvent::HandleError');
isa_ok('UniEvent::UDPError',       'UniEvent::HandleError');
isa_ok('UniEvent::StreamError',    'UniEvent::HandleError');
isa_ok('UniEvent::PipeError',      'UniEvent::StreamError');
isa_ok('UniEvent::TCPError',       'UniEvent::StreamError');
isa_ok('UniEvent::TTYError',       'UniEvent::StreamError');
isa_ok('UniEvent::SSLError',       'UniEvent::StreamError');
isa_ok('UniEvent::ResolveError',   'UniEvent::CodeError');
isa_ok('UniEvent::WorkError',      'UniEvent::CodeError');

done_testing();
