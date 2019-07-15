#pragma once
#include "Loop.h"
#include "Request.h"
#include <panda/string.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

struct DirEntry {
    enum class Type {
        UNKNOWN = UV_DIRENT_UNKNOWN,
        FILE    = UV_DIRENT_FILE,
        DIR     = UV_DIRENT_DIR,
        LINK    = UV_DIRENT_LINK,
        FIFO    = UV_DIRENT_FIFO,
        SOCKET  = UV_DIRENT_SOCKET,
        CHAR    = UV_DIRENT_CHAR,
        BLOCK   = UV_DIRENT_BLOCK
    };

    DirEntry (const string& name, Type type) : _name(name), _type(type) {}

    const string& name () const { return _name; }
    Type          type () const { return _type; }

private:
    string _name;
    Type   _type;
};

typedef std::vector<DirEntry> DirEntries;

struct FSRequest : CancelableRequest, AllocatedObject<FSRequest, true> {
    enum class State {READY, BUSY, COMPLETE};

    enum class SymlinkFlags {
        DIR      = UV_FS_SYMLINK_DIR,
        JUNCTION = UV_FS_SYMLINK_JUNCTION
    };

    enum class CopyFileFlags {
        EXCL = UV_FS_COPYFILE_EXCL
    };

    typedef function<void(FSRequest*, const CodeError*)>                    fn;
    typedef function<void(FSRequest*, const CodeError*, file_t)>            open_fn;
    typedef function<void(FSRequest*, const CodeError*, stat_t)>            stat_fn;
    typedef function<void(FSRequest*, const CodeError*, const string&)>     read_fn;
    typedef function<void(FSRequest*, const CodeError*, const DirEntries&)> scandir_fn;
    typedef function<void(FSRequest*, const CodeError*, size_t)>            sendfile_fn;

    // sync static methods
    static file_t     open     (string_view path, int flags, int mode);
    static void       close    (file_t file);
    static stat_t     stat     (string_view path);
    static stat_t     stat     (file_t file);
    static stat_t     lstat    (string_view path);
    static void       sync     (file_t file);
    static void       datasync (file_t file);
    static void       truncate (file_t file, int64_t offset);
    static void       chmod    (string_view path, int mode);
    static void       chmod    (file_t file, int mode);
    static void       utime    (string_view path, double atime, double mtime);
    static void       utime    (file_t file, double atime, double mtime);
    static void       chown    (string_view path, uid_t uid, gid_t gid);
    static void       chown    (file_t file, uid_t uid, gid_t gid);
    static void       unlink   (string_view path);
    static void       mkdir    (string_view path, int mode);
    static void       rmdir    (string_view path);
    static DirEntries scandir  (string_view path, int flags);
    static void       rename   (string_view path, string_view new_path);
    static size_t     sendfile (file_t out_fd, file_t in_fd, int64_t in_offset, size_t length);
    static void       link     (string_view path, string_view new_path);
    static void       symlink  (string_view path, string_view new_path, SymlinkFlags flags);
    static string     readlink (string_view path);
    static string     realpath (string_view path);
    static void       copyfile (string_view path, string_view new_path, CopyFileFlags flags);
    static bool       access   (string_view path, int mode);
    static string     mkdtemp  (string_view path);
    static string     read     (file_t file, size_t length, int64_t offset);
    static void       write    (file_t file, const string& buf, int64_t offset) { write(file, &buf, &buf+1, offset); }

    template <class It>
    static void write (file_t file, It begin, It end, int64_t offset) {
        size_t nbufs = end - begin;
        uv_buf_t uvbufs[nbufs];
        uv_buf_t* ptr = uvbufs;
        for (; begin != end; ++begin) {
            const string& s = *begin;
            ptr->base = const_cast<char*>(s.data()); // libuv read-only access
            ptr->len  = s.length();
        }
        _write(file, uvbufs, nbufs, offset);
    }

    // async object methods
    FSRequest (Loop* loop = Loop::default_loop()) : _state(State::READY) {
        _uvr.loop = _pex_(loop);
        _init(&_uvr);
    }

    file_t file () const { return _file; }

    void open     (file_t file) { _file = file; }
    void open     (string_view path, int flags, int mode, open_fn callback);
    void close    (fn callback);
    void stat     (string_view path, stat_fn callback);
    void stat     (stat_fn callback);
    void lstat    (string_view path, stat_fn callback);
    void sync     (fn callback);
    void datasync (fn callback);
    void truncate (int64_t offset, fn callback);
    void chmod    (string_view path, int mode, fn callback);
    void chmod    (int mode, fn callback);
    void utime    (string_view path, double atime, double mtime, fn callback);
    void utime    (double atime, double mtime, fn callback);
    void chown    (string_view path, uid_t uid, gid_t gid, fn callback);
    void chown    (uid_t uid, gid_t gid, fn callback);
    void unlink   (string_view path, fn callback);
    void mkdir    (string_view path, int mode, fn callback);
    void rmdir    (string_view path, fn callback);
    void scandir  (string_view path, int flags, scandir_fn callback);
    void rename   (string_view path, string_view new_path, fn callback);
    void sendfile (file_t out_fd, file_t in_fd, int64_t in_offset, size_t length, sendfile_fn callback);
    void link     (string_view path, string_view new_path, fn callback);
    void symlink  (string_view path, string_view new_path, SymlinkFlags flags, fn callback);
    void readlink (string_view path, read_fn callback);
    void realpath (string_view path, read_fn callback);
    void copyfile (string_view path, string_view new_path, CopyFileFlags flags, fn callback);

    void read     (size_t size, int64_t offset, read_fn callback);

    void write (const string& buf, int64_t offset, fn callback) {
        _bufs.clear();
        _bufs.push_back(buf);
        _write(offset, callback);
    }

    template <class It>
    void write (It begin, It end, int64_t offset, fn callback) {
        _bufs.clear();
        _bufs.reserve(end - begin);
        for (; begin != end; ++begin) _bufs.push_back(*begin);
        _write(offset, callback);
    }

protected:
    virtual ~FSRequest ();

private:
    uv_fs_t             _uvr;
    State               _state;
    file_t              _file;
    std::vector<string> _bufs;
    string              _read_buf;

    //    union {
        fn          _callback;
        open_fn     _open_callback;
        stat_fn     _stat_callback;
        read_fn     _read_callback;
        scandir_fn  _scandir_callback;
        sendfile_fn _sendfile_callback;
	//    };

    void _write (int64_t offset, fn callback);

    void cleanup () {
        switch (_state) {
            case State::READY: break;
            case State::BUSY:  throw Error("cannot start request while processing another");
            case State::COMPLETE:
                _state = State::READY;
                uv_fs_req_cleanup(&_uvr);
        }
    }

    void set_busy () {
        cleanup();
        _state = State::BUSY;
    }

    void set_complete () {
        _state = State::COMPLETE;
        cleanup();
    }


    static void uvx_on_complete          (uv_fs_t*);
    static void uvx_on_open_complete     (uv_fs_t*);
    static void uvx_on_stat_complete     (uv_fs_t*);
    static void uvx_on_read_complete     (uv_fs_t*);
    static void uvx_on_scandir_complete  (uv_fs_t*);
    static void uvx_on_sendfile_complete (uv_fs_t*);
    static void uvx_on_readlink_complete (uv_fs_t*);
    static void uvx_on_realpath_complete (uv_fs_t*);

    static void _write (file_t file, uv_buf_t* bufs, size_t nbufs, int64_t offset);

};

}}
