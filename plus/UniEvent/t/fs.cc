#include "lib/test.h"
#include <sys/stat.h>
#include <sys/types.h>

string root_vdir = "t/var";

struct VarDir {
    string dir;

    VarDir () {
        dir = root_vdir + "/" + string::from_number(panda::unievent::getpid());
        Fs::mkpath(dir.c_str(), 0755);
    }

    ~VarDir () {
        Fs::rmtree(dir);
    }

    string path (string_view relpath) {
        return dir + "/" + relpath;
    }
};

TEST_CASE("fs-sync", "[fs]") {
    VarDir vdir;
    auto p    = [&](string_view s) { return vdir.path(s); };
    auto file = p("file");
    auto dir  = p("dir");

    SECTION("mkdir/rmdir") {
        auto ret = Fs::mkdir(dir);
        REQUIRE(ret);
        CHECK(Fs::isdir(dir));

        ret = Fs::mkdir(dir);
        REQUIRE(!ret);
        CHECK(ret.error().code() == std::errc::file_exists);

        Fs::touch(file);
        ret = Fs::mkdir(file);
        REQUIRE(!ret);
        CHECK(ret.error().code() == std::errc::file_exists);

        CHECK(Fs::rmdir(dir));
        CHECK(!Fs::isdir(dir));

        ret = Fs::rmdir(file);
        CHECK(!ret);
        CHECK(ret.error().code() == std::errc::not_a_directory);
    }

    SECTION("mkpath") {
        auto ret = Fs::mkpath(p("dir1"));
        CHECK(ret);
        CHECK(Fs::isdir(p("dir1")));
        CHECK(Fs::mkpath(p("dir1")));
        CHECK(Fs::mkpath(p("dir2/dir3////dir4")));
        CHECK(Fs::isdir(p("dir2")));
        CHECK(Fs::isdir(p("dir2/dir3")));
        CHECK(Fs::isdir(p("dir2/dir3/dir4")));
    }

    SECTION("scandir") {
        auto ret = Fs::scandir(p("dir"));
        REQUIRE(!ret);
        CHECK(ret.error().code() == std::errc::no_such_file_or_directory);

        ret = Fs::scandir(p(""));
        REQUIRE(ret);
        CHECK(ret.value().size() == 0);

        Fs::mkdir(p("adir"));
        Fs::mkdir(p("bdir"));
        Fs::touch(p("afile"));
        Fs::touch(p("bfile"));
        ret = Fs::scandir(p(""));
        REQUIRE(ret);
        auto& list = ret.value();
        CHECK(list.size() == 4);
        CHECK(list[0].name() == "adir");
        CHECK(list[0].type() == Fs::FileType::DIR);
        CHECK(list[1].name() == "afile");
        CHECK(list[1].type() == Fs::FileType::FILE);
        CHECK(list[2].name() == "bdir");
        CHECK(list[2].type() == Fs::FileType::DIR);
        CHECK(list[3].name() == "bfile");
        CHECK(list[3].type() == Fs::FileType::FILE);
    }

    SECTION("rmtree") {
        Fs::mkpath(p("dir/dir1/dir2/dir3"));
        Fs::mkpath(p("dir/dir4"));
        Fs::touch(p("dir/file1"));
        Fs::touch(p("dir/file2"));
        Fs::touch(p("dir/dir4/file3"));
        Fs::touch(p("dir/dir1/file4"));
        Fs::touch(p("dir/dir1/dir2/file5"));
        Fs::touch(p("dir/dir1/dir2/dir3/file6"));
        CHECK(Fs::rmtree(p("dir")));
        CHECK(!Fs::exists(p("dir")));
    }

    SECTION("open/close") {
        auto ret = Fs::open(file, Fs::OpenFlags::RDONLY);
        REQUIRE(!ret);
        CHECK(ret.error().code() == std::errc::no_such_file_or_directory);

        ret = Fs::open(file, Fs::OpenFlags::RDWR | Fs::OpenFlags::CREAT);
        CHECK(ret.value());

        auto cret = Fs::close(*ret);
        CHECK(cret);

        ret = Fs::open(file, Fs::OpenFlags::RDONLY);
        CHECK(ret);
        Fs::close(*ret);
    }

    SECTION("stat") {
        auto ret = Fs::stat(file);
        CHECK(!ret);

        Fs::touch(file);

        ret = Fs::stat(file);
        REQUIRE(ret);
        auto s = ret.value();
        CHECK(s.mtime.get());
        CHECK(s.type() == Fs::FileType::FILE);

        auto fd = Fs::open(file, Fs::OpenFlags::RDONLY).value();
        ret = Fs::stat(fd);
        REQUIRE(ret);
        CHECK(ret.value().type() == Fs::FileType::FILE);
    }

    SECTION("exists/isfile/isdir") {
        CHECK(!Fs::exists(file));
        CHECK(!Fs::isfile(file));
        CHECK(!Fs::isdir(file));
        Fs::touch(file);
        CHECK(Fs::exists(file));
        CHECK(Fs::isfile(file));
        CHECK(!Fs::isdir(file));

        Fs::mkdir(dir);
        CHECK(Fs::exists(dir));
        CHECK(!Fs::isfile(dir));
        CHECK(Fs::isdir(dir));
    }

    SECTION("unlink") {
        auto ret = Fs::unlink(file);
        REQUIRE(!ret);
        CHECK(ret.error().code() == std::errc::no_such_file_or_directory);

        Fs::touch(file);
        CHECK(Fs::unlink(file));
        CHECK(!Fs::exists(file));

        Fs::mkdir(dir);
        ret = Fs::unlink(dir);
        REQUIRE(!ret);
        CHECK(ret.error().code() == std::errc::is_a_directory);
    }

}
