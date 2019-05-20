#pragma once
#include "Loop.h"
#include "Request.h"
#include <panda/excepted.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

struct Fs : Work, lib::AllocatedObject<Fs> {
    template <class T>
    using ex = excepted<T, CodeError>;

    static const int DEFAULT_FILE_MODE = 0644;
    static const int DEFAULT_DIR_MODE  = 0755;

    struct OpenFlags {
        static const int APPEND;
        static const int CREAT;
        static const int DIRECT;
        static const int DIRECTORY;
        static const int DSYNC;
        static const int EXCL;
        static const int EXLOCK;
        static const int NOATIME;
        static const int NOCTTY;
        static const int NOFOLLOW;
        static const int NONBLOCK;
        static const int RANDOM;
        static const int RDONLY;
        static const int RDWR;
        static const int SEQUENTIAL;
        static const int SHORT_LIVED;
        static const int SYMLINK;
        static const int SYNC;
        static const int TEMPORARY;
        static const int TRUNC;
        static const int WRONLY;
    };

    enum class SymlinkFlags  {DIR, JUNCTION};
    enum class CopyFileFlags {EXCL, FICLONE, FICLONE_FORCE};
    enum class FileType      {BLOCK, CHAR, DIR, FIFO, LINK, FILE, SOCKET, UNKNOWN};

    struct Stat {
      uint64_t dev;
      uint64_t mode;
      uint64_t nlink;
      uint64_t uid;
      uint64_t gid;
      uint64_t rdev;
      uint64_t ino;
      uint64_t size;
      uint64_t blksize;
      uint64_t blocks;
      uint64_t flags;
      uint64_t gen;
      TimeSpec atime;
      TimeSpec mtime;
      TimeSpec ctime;
      TimeSpec birthtime;

      FileType type () const { return ftype(mode); }
    };

    struct DirEntry {
        DirEntry (const string& name, FileType type) : _name(name), _type(type) {}

        const string& name () const { return _name; }
        FileType      type () const { return _type; }

    private:
        string   _name;
        FileType _type;
    };
    using DirEntries = std::vector<DirEntry>;

//    typedef function<void(FSRequest*, const CodeError*)>                    fn;
//    typedef function<void(FSRequest*, const CodeError*, fd_t)>              open_fn;
//    typedef function<void(FSRequest*, const CodeError*, stat_t)>            stat_fn;
//    typedef function<void(FSRequest*, const CodeError*, const string&)>     read_fn;
//    typedef function<void(FSRequest*, const CodeError*, const DirEntries&)> scandir_fn;
//    typedef function<void(FSRequest*, const CodeError*, size_t)>            sendfile_fn;

    static FileType ftype (uint64_t mode);

    // sync static methods
    static ex<void>       mkdir    (string_view path, int mode = DEFAULT_DIR_MODE);
    static ex<void>       rmdir    (string_view path);
    static ex<void>       mkpath   (string_view path, int mode = DEFAULT_DIR_MODE);
    static ex<DirEntries> scandir  (string_view path);
    static ex<void>       rmtree   (string_view path);

    static ex<fd_t>       open     (std::string_view path, int flags, int mode = DEFAULT_FILE_MODE);
    static ex<void>       close    (fd_t file);
    static ex<Stat>       stat     (string_view path);
    static ex<Stat>       stat     (fd_t file);
    static ex<Stat>       lstat    (string_view path);
    static bool           exists   (string_view path);
    static bool           isfile   (string_view path);
    static bool           isdir    (string_view path);
    static ex<void>       unlink   (string_view path);
    static ex<void>       sync     (fd_t file);
    static ex<void>       datasync (fd_t file);
    static ex<void>       truncate (fd_t file, int64_t offset = 0);
    static ex<void>       chmod    (string_view path, int mode);
    static ex<void>       chmod    (fd_t file, int mode);
    static ex<void>       touch    (string_view path, int mode = DEFAULT_FILE_MODE);
    static ex<void>       utime    (string_view path, double atime, double mtime);
    static ex<void>       utime    (fd_t file, double atime, double mtime);
    static ex<void>       chown    (string_view path, uid_t uid, gid_t gid);
    static ex<void>       chown    (fd_t file, uid_t uid, gid_t gid);

//    static ex<void>       rename   (string_view path, string_view new_path);
//    static size_t     sendfile (fd_t out_fd, fd_t in_fd, int64_t in_offset, size_t length);
//    static ex<void>       link     (string_view path, string_view new_path);
//    static ex<void>       symlink  (string_view path, string_view new_path, SymlinkFlags flags);
//    static string     readlink (string_view path);
//    static string     realpath (string_view path);
//    static ex<void>       copyfile (string_view path, string_view new_path, CopyFileFlags flags);
//    static bool       access   (string_view path, int mode);
//    static string     mkdtemp  (string_view path);
    static ex<string>     read     (fd_t file, size_t length, int64_t offset = 0);
//    static ex<void>       write    (fd_t file, const string& buf, int64_t offset) { write(file, &buf, &buf+1, offset); }
//
//    template <class It>
//    static void write (fd_t file, It begin, It end, int64_t offset) {
//        size_t nbufs = end - begin;
//        uv_buf_t uvbufs[nbufs];
//        uv_buf_t* ptr = uvbufs;
//        for (; begin != end; ++begin) {
//            const string& s = *begin;
//            ptr->base = const_cast<char*>(s.data()); // libuv read-only access
//            ptr->len  = s.length();
//        }
//        _write(file, uvbufs, nbufs, offset);
//    }

//    // async object methods
//    FSRequest (Loop* loop = Loop::default_loop()) : _state(State::READY) {
//        _uvr.loop = _pex_(loop);
//        _init(&_uvr);
//    }

//    fd_t file () const { return _file; }

//    void open     (fd_t file) { _file = file; }
//    void open     (string_view path, int flags, int mode, open_fn callback);
//    void close    (fn callback);
//    void stat     (string_view path, stat_fn callback);
//    void stat     (stat_fn callback);
//    void lstat    (string_view path, stat_fn callback);
//    void sync     (fn callback);
//    void datasync (fn callback);
//    void truncate (int64_t offset, fn callback);
//    void chmod    (string_view path, int mode, fn callback);
//    void chmod    (int mode, fn callback);
//    void utime    (string_view path, double atime, double mtime, fn callback);
//    void utime    (double atime, double mtime, fn callback);
//    void chown    (string_view path, uid_t uid, gid_t gid, fn callback);
//    void chown    (uid_t uid, gid_t gid, fn callback);
//    void unlink   (string_view path, fn callback);
//    void mkdir    (string_view path, int mode, fn callback);
//    void rmdir    (string_view path, fn callback);
//    void scandir  (string_view path, int flags, scandir_fn callback);
//    void rename   (string_view path, string_view new_path, fn callback);
//    void sendfile (fd_t out_fd, fd_t in_fd, int64_t in_offset, size_t length, sendfile_fn callback);
//    void link     (string_view path, string_view new_path, fn callback);
//    void symlink  (string_view path, string_view new_path, SymlinkFlags flags, fn callback);
//    void readlink (string_view path, read_fn callback);
//    void realpath (string_view path, read_fn callback);
//    void copyfile (string_view path, string_view new_path, CopyFileFlags flags, fn callback);
//
//    void read     (size_t size, int64_t offset, read_fn callback);
//
//    void write (const string& buf, int64_t offset, fn callback) {
//        _bufs.clear();
//        _bufs.push_back(buf);
//        _write(offset, callback);
//    }
//
//    template <class It>
//    void write (It begin, It end, int64_t offset, fn callback) {
//        _bufs.clear();
//        _bufs.reserve(end - begin);
//        for (; begin != end; ++begin) _bufs.push_back(*begin);
//        _write(offset, callback);
//    }

protected:
//    virtual ~FSRequest ();

private:
    enum class State {READY, BUSY, COMPLETE};
//    uv_fs_t             _uvr;
//    State               _state;
//    fd_t                _file;
//    std::vector<string> _bufs;
//    string              _read_buf;
//
//    //    union {
//        fn          _callback;
//        open_fn     _open_callback;
//        stat_fn     _stat_callback;
//        read_fn     _read_callback;
//        scandir_fn  _scandir_callback;
//        sendfile_fn _sendfile_callback;
//	//    };
//
//    void _write (int64_t offset, fn callback);
//
//    void cleanup () {
//        switch (_state) {
//            case State::READY: break;
//            case State::BUSY:  throw Error("cannot start request while processing another");
//            case State::COMPLETE:
//                _state = State::READY;
//                uv_fs_req_cleanup(&_uvr);
//        }
//    }
//
//    void set_busy () {
//        cleanup();
//        _state = State::BUSY;
//    }
//
//    void set_complete () {
//        _state = State::COMPLETE;
//        cleanup();
//    }


//    static void uvx_on_complete          (uv_fs_t*);
//    static void uvx_on_open_complete     (uv_fs_t*);
//    static void uvx_on_stat_complete     (uv_fs_t*);
//    static void uvx_on_read_complete     (uv_fs_t*);
//    static void uvx_on_scandir_complete  (uv_fs_t*);
//    static void uvx_on_sendfile_complete (uv_fs_t*);
//    static void uvx_on_readlink_complete (uv_fs_t*);
//    static void uvx_on_realpath_complete (uv_fs_t*);
//
//    static void _write (fd_t file, uv_buf_t* bufs, size_t nbufs, int64_t offset);

};

}}
