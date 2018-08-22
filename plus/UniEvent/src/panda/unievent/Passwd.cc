#include <panda/unievent/Passwd.h>
using namespace panda::unievent;

Passwd::~Passwd () {
	uv_os_free_passwd(&data);
}
