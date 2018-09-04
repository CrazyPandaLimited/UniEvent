#include <panda/unievent/StreamFilter.h>
#include <panda/unievent/Stream.h>
using namespace panda::unievent;

// Default behavior implementation - keep default behavior of handle

const char* StreamFilter::TYPE = "DEFAULT";

void StreamFilter::accept      (Stream*)                                   {}
void StreamFilter::write       (WriteRequest* req)                         { next_write(req); }
void StreamFilter::on_connect  (const CodeError* err, ConnectRequest* req) { next_on_connect(err, req); }
void StreamFilter::on_write    (const CodeError* err, WriteRequest* req)   { next_on_write(err, req); }
void StreamFilter::on_read     (const string& buf, const CodeError* err)   { next_on_read(buf, err); }
void StreamFilter::on_shutdown (const CodeError*, ShutdownRequest*)        {}
void StreamFilter::on_eof      ()                                          {}
void StreamFilter::reset       ()                                          {}
bool StreamFilter::is_secure   ()                                          { return false; }
