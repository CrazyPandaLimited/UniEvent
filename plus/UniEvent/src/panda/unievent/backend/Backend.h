#include "BackendLoop.h"

namespace panda { namespace unievent { namespace backend {

struct Backend {

    panda::string type () const { return _type; }

    virtual BackendLoop* new_loop         () = 0;
    virtual BackendLoop* new_global_loop  () = 0;
    virtual BackendLoop* new_default_loop () = 0;

    virtual ~Backend () {}

protected:
    Backend (std::string_view type) {}

private:
    panda::string _type;
};

}}}
