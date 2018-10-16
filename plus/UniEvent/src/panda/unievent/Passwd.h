#pragma once
#include "Error.h"

namespace panda { namespace unievent {

struct Passwd {
    Passwd () {
        int err = uv_os_get_passwd(&data);
        if (err) throw CodeError(err);
    }
    
    const char* username () const { return data.username; }
    long        uid      () const { return data.uid; }
    long        gid      () const { return data.gid; }
    const char* shell    () const { return data.shell; }
    const char* homedir  () const { return data.homedir; }

    virtual ~Passwd ();

private:
    uv_passwd_t data;

};

}}
