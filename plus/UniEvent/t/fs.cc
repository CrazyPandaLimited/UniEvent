#ifndef _WIN32
#include "lib/test.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

string root_vdir = "t/var";

struct VarDir {
    string vdir;

    VarDir () {
        vdir = root_vdir + "/" + string::from_number(getpid());
        auto ret = mkdir(vdir.c_str(), 0777);
        assert(!ret || errno == EEXIST);
    }

    ~VarDir () {
        rmdir(vdir.c_str());
        rmdir(root_vdir.c_str());
    }

    bool exists (string relpath) {
        auto path = vdir + "/" + relpath;
        struct stat s;
        auto ret = stat(path.c_str(), &s);
        return !ret;
    }

    file_t create_file (string relpath) {
        auto path = vdir + "/" + relpath;
        auto fp = fopen(path.c_str(), "w");
        assert(fp);
        auto ret = fclose(fp);
        assert(!ret);
    }
};

TEST_CASE("fs-sync", "[fs]") {
    VarDir vdir;

    SECTION("open/close") {
        auto ret = Fs::open("file", 0, O_RDONLY);
    }
}

#endif
