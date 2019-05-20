#include "Fs.h"
#include <uv.h>
#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>

using namespace panda::unievent;

template <class T>
using ex = Fs::ex<T>;

#ifdef _WIN32
    #define UE_SLASH '\\'
#else
    #define UE_SLASH '/'
#endif

const int Fs::OpenFlags::APPEND      = UV_FS_O_APPEND;
const int Fs::OpenFlags::CREAT       = UV_FS_O_CREAT;
const int Fs::OpenFlags::DIRECT      = UV_FS_O_DIRECT;
const int Fs::OpenFlags::DIRECTORY   = UV_FS_O_DIRECTORY;
const int Fs::OpenFlags::DSYNC       = UV_FS_O_DSYNC;
const int Fs::OpenFlags::EXCL        = UV_FS_O_EXCL;
const int Fs::OpenFlags::EXLOCK      = UV_FS_O_EXLOCK;
const int Fs::OpenFlags::NOATIME     = UV_FS_O_NOATIME;
const int Fs::OpenFlags::NOCTTY      = UV_FS_O_NOCTTY;
const int Fs::OpenFlags::NOFOLLOW    = UV_FS_O_NOFOLLOW;
const int Fs::OpenFlags::NONBLOCK    = UV_FS_O_NONBLOCK;
const int Fs::OpenFlags::RANDOM      = UV_FS_O_RANDOM;
const int Fs::OpenFlags::RDONLY      = UV_FS_O_RDONLY;
const int Fs::OpenFlags::RDWR        = UV_FS_O_RDWR;
const int Fs::OpenFlags::SEQUENTIAL  = UV_FS_O_SEQUENTIAL;
const int Fs::OpenFlags::SHORT_LIVED = UV_FS_O_SHORT_LIVED;
const int Fs::OpenFlags::SYMLINK     = UV_FS_O_SYMLINK;
const int Fs::OpenFlags::SYNC        = UV_FS_O_SYNC;
const int Fs::OpenFlags::TEMPORARY   = UV_FS_O_TEMPORARY;
const int Fs::OpenFlags::TRUNC       = UV_FS_O_TRUNC;
const int Fs::OpenFlags::WRONLY      = UV_FS_O_WRONLY;

static inline void uvx_ts2ue (const uv_timespec_t& from, TimeSpec& to) {
    to.sec  = from.tv_sec;
    to.nsec = from.tv_nsec;
}

static inline void uvx_stat2ue (const uv_stat_t* from, Fs::Stat& to) {
    to.dev     = from->st_dev;
    to.mode    = from->st_mode;
    to.nlink   = from->st_nlink;
    to.uid     = from->st_uid;
    to.gid     = from->st_gid;
    to.rdev    = from->st_rdev;
    to.ino     = from->st_ino;
    to.size    = from->st_size;
    to.blksize = from->st_blksize;
    to.blocks  = from->st_blocks;
    to.flags   = from->st_flags;
    to.gen     = from->st_gen;
    uvx_ts2ue(from->st_atim, to.atime);
    uvx_ts2ue(from->st_mtim, to.mtime);
    uvx_ts2ue(from->st_ctim, to.ctime);
    uvx_ts2ue(from->st_birthtim, to.birthtime);
}

Fs::FileType Fs::ftype (uint64_t mode) {
    #ifdef _WIN32
        switch (mode & _S_IFMT) {
            case _S_IFDIR: return FileType::DIR;
            case _S_IFCHR: return FileType::CHAR;
            case _S_IFREG: return FileType::FILE;
        }
    #else
        switch (mode & S_IFMT) {
            case S_IFBLK:  return FileType::BLOCK;
            case S_IFCHR:  return FileType::CHAR;
            case S_IFDIR:  return FileType::DIR;
            case S_IFIFO:  return FileType::FIFO;
            case S_IFLNK:  return FileType::LINK;
            case S_IFREG:  return FileType::FILE;
            case S_IFSOCK: return FileType::SOCKET;

        }
    #endif
    return FileType::UNKNOWN;
}

static inline Fs::FileType uvx_ftype (uv_dirent_type_t uvt) {
    switch (uvt) {
        case UV_DIRENT_UNKNOWN : return Fs::FileType::UNKNOWN;
        case UV_DIRENT_FILE    : return Fs::FileType::FILE;
        case UV_DIRENT_DIR     : return Fs::FileType::DIR;
        case UV_DIRENT_LINK    : return Fs::FileType::LINK;
        case UV_DIRENT_FIFO    : return Fs::FileType::FIFO;
        case UV_DIRENT_SOCKET  : return Fs::FileType::SOCKET;
        case UV_DIRENT_CHAR    : return Fs::FileType::CHAR;
        case UV_DIRENT_BLOCK   : return Fs::FileType::BLOCK;
    }
    abort(); // not reachable
}

//=================================== SYNC API ==================================================

#define UEFS_CALL_SYNC_RAW(code) { \
    uv_fs_t uvr;                   \
    uvr.loop = nullptr;            \
    code                           \
    uv_fs_req_cleanup(&uvr);       \
}

#define UEFS_CALL_SYNC(call_code, result_code) {                                \
    UEFS_CALL_SYNC_RAW({                                                        \
        call_code                                                               \
        if (uvr.result < 0) return make_unexpected(uvx_code_error(uvr.result)); \
        result_code                                                             \
    })                                                                          \
}

ex<void> Fs::mkdir (string_view path, int mode) {
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_mkdir(nullptr, &uvr, path_str, mode, nullptr);
    }, {});
    return {};
}

ex<void> Fs::rmdir (string_view path) {
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_rmdir(nullptr, &uvr, path_str, nullptr);
    }, {});
    return {};
}

ex<void> Fs::mkpath (string_view path, int mode) {
    auto len = path.length();
    if (!len) return {};
    size_t pos = 0;

    auto skip_slash = [&]() { while (pos < len && (path[pos] == '/' || path[pos] == '\\')) ++pos; };
    auto find_part  = [&]() { while (pos < len && path[pos] != '/' && path[pos] != '\\') ++pos; };

    #ifdef _WIN32
      auto dpos = path.find_first_of(':');
      if (dpos != string_view::npos) pos = dpos + 1;
    #endif
    skip_slash();

    // root folder ('/') or drives ('C:\') always exist
    while (pos < len) {
        find_part();
        auto ret = mkdir(path.substr(0, pos), mode);
        if (!ret && ret.error().code() != std::errc::file_exists) return ret;
        skip_slash();
    }

    return {};
}

ex<Fs::DirEntries> Fs::scandir (string_view path) {
    DirEntries ret;
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_scandir(nullptr, &uvr, path_str, 0, nullptr);
    }, {
        size_t cnt = (size_t)uvr.result;
        if (cnt) {
            ret.reserve(cnt);
            uv_dirent_t uvent;
            while (uv_fs_scandir_next(&uvr, &uvent) == 0) ret.emplace(ret.cend(), string(uvent.name), uvx_ftype(uvent.type));
        }
    });
    return std::move(ret);
}

ex<void> Fs::rmtree (string_view path) {
    auto plen = path.length();
    return scandir(path).and_then([&](const DirEntries& entries) {
        for (const auto& entry : entries) {
            auto elen = entry.name().length();
            auto fnlen = plen + elen + 1;
            char _fn[fnlen];
            char* ptr = _fn;
            std::memcpy(ptr, path.data(), plen);
            ptr += plen;
            *ptr++ = UE_SLASH;
            std::memcpy(ptr, entry.name().data(), elen);

            string_view fname(_fn, fnlen);
            if (entry.type() == FileType::DIR) {
                auto ret = rmtree(fname);
                if (!ret) return ret;
            } else {
                auto ret = unlink(fname);
                if (!ret) return ret;
            }
        }
        return rmdir(path);
    });
}

ex<fd_t> Fs::open (string_view path, int flags, int mode) {
    fd_t ret;
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_open(nullptr, &uvr, path_str, flags, mode, nullptr);
    }, {
        ret = (fd_t)uvr.result;
    });
    return ret;
}

ex<void> Fs::close (fd_t file) {
    UEFS_CALL_SYNC({
        uv_fs_close(nullptr, &uvr, file, nullptr);
    }, {});
    return {};
}

ex<Fs::Stat> Fs::stat (string_view path) {
    Stat ret;
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_stat(nullptr, &uvr, path_str, nullptr);
    }, {
        uvx_stat2ue(&uvr.statbuf, ret);
    });
    return ret;
}

ex<Fs::Stat> Fs::stat (fd_t file) {
    Stat ret;
    UEFS_CALL_SYNC({
        uv_fs_fstat(nullptr, &uvr, file, nullptr);
    }, {
        uvx_stat2ue(&uvr.statbuf, ret);
    });
    return ret;
}

ex<Fs::Stat> Fs::lstat (string_view path) {
    Stat ret;
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_lstat(nullptr, &uvr, path_str, nullptr);
    }, {
        uvx_stat2ue(&uvr.statbuf, ret);
    });
    return ret;
}

bool Fs::exists (string_view file) {
    return (bool)stat(file);
}

bool Fs::isfile (string_view file) {
    return stat(file).map([](const Stat& s) {
        return s.type() == FileType::FILE;
    }).value_or(false);
}

bool Fs::isdir (string_view file) {
    return stat(file).map([](const Stat& s) {
        return s.type() == FileType::DIR;
    }).value_or(false);
}

ex<void> Fs::unlink (string_view path) {
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_unlink(nullptr, &uvr, path_str, nullptr);
    }, {});
    return {};
}

ex<void> Fs::sync (fd_t file) {
    UEFS_CALL_SYNC({
        uv_fs_fsync(nullptr, &uvr, file, nullptr);
    }, {});
    return {};
}

ex<void> Fs::datasync (fd_t file) {
    UEFS_CALL_SYNC({
        uv_fs_fdatasync(nullptr, &uvr, file, nullptr);
    }, {});
    return {};
}

ex<void> Fs::truncate (fd_t file, int64_t offset) {
    UEFS_CALL_SYNC({
        uv_fs_ftruncate(nullptr, &uvr, file, offset, nullptr);
    }, {});
    return {};
}

ex<void> Fs::chmod (string_view path, int mode) {
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_chmod(nullptr, &uvr, path_str, mode, nullptr);
    }, {});
    return {};
}

ex<void> Fs::chmod (fd_t file, int mode) {
    UEFS_CALL_SYNC({
        uv_fs_fchmod(nullptr, &uvr, file, mode, nullptr);
    }, {});
    return {};
}

ex<void> Fs::touch (string_view file, int mode) {
    return open(file, OpenFlags::RDWR | OpenFlags::CREAT, mode).and_then(close);
}

ex<void> Fs::utime (string_view path, double atime, double mtime) {
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_utime(nullptr, &uvr, path_str, atime, mtime, nullptr);
    }, {});
    return {};
}

ex<void> Fs::utime (fd_t file, double atime, double mtime) {
    UEFS_CALL_SYNC({
        uv_fs_futime(nullptr, &uvr, file, atime, mtime, nullptr);
    }, {});
    return {};
}

ex<void> Fs::chown (string_view path, uid_t uid, gid_t gid) {
    UE_NULL_TERMINATE(path, path_str);
    UEFS_CALL_SYNC({
        uv_fs_chown(nullptr, &uvr, path_str, uid, gid, nullptr);
    }, {});
    return {};
}

ex<void> Fs::chown (fd_t file, uid_t uid, gid_t gid) {
    UEFS_CALL_SYNC({
        uv_fs_fchown(nullptr, &uvr, file, uid, gid, nullptr);
    }, {});
    return {};
}

ex<string> Fs::read (fd_t file, size_t length, int64_t offset) {
    string ret;
    char* ptr = ret.reserve(length);
    uv_buf_t uvbuf;
    uvbuf.base = ptr;
    uvbuf.len  = length;
    UEFS_CALL_SYNC({
        uv_fs_read(nullptr, &uvr, file, &uvbuf, 1, offset, nullptr);
    }, {
        ret.length(length);
    });
    return ret;
}

//void FSRequest::_write (fd_t file, uv_buf_t* uvbufs, size_t nbufs, int64_t offset) {
//    PE_FS_CALL_SYNC({
//        uv_fs_write(_uvr.loop, &_uvr, file, uvbufs, nbufs, offset, nullptr);
//    }, {});
//}
//
//void FSRequest::rename (string_view path, string_view new_path) {
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_rename(_uvr.loop, &_uvr, path_str, new_path_str, nullptr);
//    }, {});
//}
//
//size_t FSRequest::sendfile (fd_t out_fd, fd_t in_fd, int64_t in_offset, size_t length) {
//    size_t ret;
//    PE_FS_CALL_SYNC({
//        uv_fs_sendfile(_uvr.loop, &_uvr, out_fd, in_fd, in_offset, length, nullptr);
//    }, {
//        ret = (size_t)_uvr.result;
//    });
//    return ret;
//}
//
//void FSRequest::link (string_view path, string_view new_path) {
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_link(_uvr.loop, &_uvr, path_str, new_path_str, nullptr);
//    }, {});
//}
//
//void FSRequest::symlink (string_view path, string_view new_path, SymlinkFlags flags) {
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_symlink(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, nullptr);
//    }, {});
//}
//
//string FSRequest::readlink (string_view path) {
//    string ret;
//    UE_NULL_TERMINATE(path, path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_readlink(_uvr.loop, &_uvr, path_str, nullptr);
//    }, {
//        ret.assign((const char*)_uvr.ptr, (size_t)_uvr.result); // _uvr.ptr is not null-terminated
//    });
//    return ret;
//}
//
//string FSRequest::realpath (string_view path) {
//    string ret;
//    UE_NULL_TERMINATE(path, path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_realpath(_uvr.loop, &_uvr, path_str, nullptr);
//    }, {
//        ret.assign((const char*)_uvr.ptr); // _uvr.ptr is null-terminated
//    });
//    return ret;
//}
//
//void FSRequest::copyfile (string_view path, string_view new_path, CopyFileFlags flags) {
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_copyfile(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, nullptr);
//    }, {});
//}
//
//bool FSRequest::access (string_view path, int mode) {
//    bool ret;
//    UE_NULL_TERMINATE(path, path_str);
//    PE_FS_CALL_SYNC_NOERR({
//        uv_fs_access(_uvr.loop, &_uvr, path_str, mode, nullptr);
//        switch (_uvr.result) {
//            case 0:
//                ret = true;
//                break;
//            case UV_EACCES:
//            case UV_EROFS:
//                ret = false;
//                break;
//            default:
//                throw CodeError(_uvr.result);
//        }
//    });
//    return ret;
//}
//
//string FSRequest::mkdtemp (string_view path) {
//    string ret;
//    UE_NULL_TERMINATE(path, path_str);
//    PE_FS_CALL_SYNC({
//        uv_fs_mkdtemp(_uvr.loop, &_uvr, path_str, nullptr);
//    }, {
//        ret.assign(_uvr.path);
//    });
//    return ret;
//}
//
//
//void FSRequest::uvx_on_open_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    int status = 0;
//    if (req->_uvr.result >= 0) req->_file = req->_uvr.result;
//    else                       status = req->_uvr.result;
//    req->set_complete();
//    req->_open_callback(req, CodeError(status), req->_file);
//    req->release();
//}
//
//void FSRequest::open (string_view path, int flags, int mode, open_fn callback) {
//    set_busy();
//    _open_callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_open(_uvr.loop, &_uvr, path_str, flags, mode, uvx_on_open_complete);
//}
//
//void FSRequest::uvx_on_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    CodeError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
//    req->set_complete();
//    req->_callback(req, err);
//    req->release();
//}
//
//void FSRequest::close (fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_close(_uvr.loop, &_uvr, _file, uvx_on_complete);
//}
//
//void FSRequest::uvx_on_stat_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    stat_t val;
//    int status = 0;
//    if (req->_uvr.result >= 0) val = req->_uvr.statbuf;
//    else                       status = req->_uvr.result;
//    req->set_complete();
//    req->_stat_callback(req, CodeError(status), val);
//    req->release();
//}
//
//void FSRequest::stat (string_view path, stat_fn callback) {
//    set_busy();
//    _stat_callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_stat(_uvr.loop, &_uvr, path_str, uvx_on_stat_complete);
//}
//
//void FSRequest::stat (stat_fn callback) {
//    set_busy();
//    _stat_callback = callback;
//    retain();
//    uv_fs_fstat(_uvr.loop, &_uvr, _file, uvx_on_stat_complete);
//}
//
//void FSRequest::lstat (string_view path, stat_fn callback) {
//    set_busy();
//    _stat_callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_lstat(_uvr.loop, &_uvr, path_str, uvx_on_stat_complete);
//}
//
//void FSRequest::sync (fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_fsync(_uvr.loop, &_uvr, _file, uvx_on_complete);
//}
//
//void FSRequest::datasync (fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_fdatasync(_uvr.loop, &_uvr, _file, uvx_on_complete);
//}
//
//void FSRequest::truncate (int64_t offset, fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_ftruncate(_uvr.loop, &_uvr, _file, offset, uvx_on_complete);
//}
//
//void FSRequest::chmod (string_view path, int mode, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_chmod(_uvr.loop, &_uvr, path_str, mode, uvx_on_complete);
//}
//
//void FSRequest::chmod (int mode, fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_fchmod(_uvr.loop, &_uvr, _file, mode, uvx_on_complete);
//}
//
//void FSRequest::utime (string_view path, double atime, double mtime, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_utime(_uvr.loop, &_uvr, path_str, atime, mtime, uvx_on_complete);
//}
//
//void FSRequest::utime (double atime, double mtime, fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_futime(_uvr.loop, &_uvr, _file, atime, mtime, uvx_on_complete);
//}
//
//void FSRequest::chown (string_view path, uid_t uid, gid_t gid, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_chown(_uvr.loop, &_uvr, path_str, uid, gid, uvx_on_complete);
//}
//
//void FSRequest::chown (uid_t uid, gid_t gid, fn callback) {
//    set_busy();
//    _callback = callback;
//    retain();
//    uv_fs_fchown(_uvr.loop, &_uvr, _file, uid, gid, uvx_on_complete);
//}
//
//void FSRequest::unlink (string_view path, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_unlink(_uvr.loop, &_uvr, path_str, uvx_on_complete);
//}
//
//void FSRequest::mkdir (string_view path, int mode, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_mkdir(_uvr.loop, &_uvr, path_str, mode, uvx_on_complete);
//}
//
//void FSRequest::rmdir (string_view path, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_rmdir(_uvr.loop, &_uvr, path_str, uvx_on_complete);
//}
//
//void FSRequest::uvx_on_scandir_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    int status = 0;
//    DirEntries ret;
//    if (req->_uvr.result >= 0) {
//        size_t nent = (size_t)req->_uvr.result;
//        if (nent) {
//            ret.reserve(nent);
//            auto uv_entries = (uv_dirent_t**)req->_uvr.ptr;
//            for (size_t i = 0; i < nent; ++i) ret.emplace(ret.cend(), string(uv_entries[i]->name), (DirEntry::Type)uv_entries[i]->type);
//        }
//    }
//    else status = req->_uvr.result;
//    req->set_complete();
//    req->_scandir_callback(req, CodeError(status), ret);
//    req->release();
//}
//
//void FSRequest::scandir (string_view path, int flags, scandir_fn callback) {
//    set_busy();
//    _scandir_callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_scandir(_uvr.loop, &_uvr, path_str, flags, uvx_on_scandir_complete);
//}
//
//void FSRequest::rename (string_view path, string_view new_path, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    retain();
//    uv_fs_rename(_uvr.loop, &_uvr, path_str, new_path_str, uvx_on_complete);
//}
//
//void FSRequest::uvx_on_sendfile_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    int status = 0;
//    size_t ret = 0;
//    if (req->_uvr.result >= 0) ret = (size_t)req->_uvr.result;
//    else                       status = req->_uvr.result;
//    req->set_complete();
//    req->_sendfile_callback(req, CodeError(status), ret);
//    req->release();
//}
//
//void FSRequest::sendfile (fd_t out_fd, fd_t in_fd, int64_t in_offset, size_t length, sendfile_fn callback) {
//    set_busy();
//    _sendfile_callback = callback;
//    retain();
//    uv_fs_sendfile(_uvr.loop, &_uvr, out_fd, in_fd, in_offset, length, uvx_on_sendfile_complete);
//}
//
//void FSRequest::link (string_view path, string_view new_path, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    retain();
//    uv_fs_link(_uvr.loop, &_uvr, path_str, new_path_str, uvx_on_complete);
//}
//
//void FSRequest::symlink (string_view path, string_view new_path, SymlinkFlags flags, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    retain();
//    uv_fs_symlink(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, uvx_on_complete);
//}
//
//void FSRequest::uvx_on_readlink_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    int status = 0;
//    string ret;
//    if (req->_uvr.result >= 0) ret.assign((const char*)req->_uvr.ptr, (size_t)req->_uvr.result); // _uvr.ptr is not null-terminated
//    else                       status = req->_uvr.result;
//    req->set_complete();
//    req->_read_callback(req, CodeError(status), ret);
//    req->release();
//}
//
//void FSRequest::readlink (string_view path, read_fn callback) {
//    set_busy();
//    _read_callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_readlink(_uvr.loop, &_uvr, path_str, uvx_on_readlink_complete);
//}
//
//void FSRequest::uvx_on_realpath_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    int status = 0;
//    string ret;
//    if (req->_uvr.result >= 0) ret.assign((const char*)req->_uvr.ptr); // _uvr.ptr is null-terminated
//    else                       status = req->_uvr.result;
//    req->set_complete();
//    req->_read_callback(req, CodeError(status), ret);
//    req->release();
//}
//
//void FSRequest::realpath (string_view path, read_fn callback) {
//    set_busy();
//    _read_callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    retain();
//    uv_fs_realpath(_uvr.loop, &_uvr, path_str, uvx_on_realpath_complete);
//}
//
//void FSRequest::copyfile (string_view path, string_view new_path, CopyFileFlags flags, fn callback) {
//    set_busy();
//    _callback = callback;
//    UE_NULL_TERMINATE(path, path_str);
//    UE_NULL_TERMINATE(new_path, new_path_str);
//    retain();
//    uv_fs_copyfile(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, uvx_on_complete);
//}
//
//void FSRequest::uvx_on_read_complete (uv_fs_t* uvreq) {
//    auto req = rcast<FSRequest*>(uvreq);
//    int status = 0;
//    if (req->_uvr.result >= 0) req->_read_buf.length((size_t)req->_uvr.result);
//    else                       status = req->_uvr.result;
//    req->set_complete();
//    req->_read_callback(req, CodeError(status), req->_read_buf);
//    req->_read_buf.clear();
//    req->release();
//}
//
//void FSRequest::read (size_t size, int64_t offset, read_fn callback) {
//    set_busy();
//    _read_callback = callback;
//    _read_buf.clear();
//    char* ptr = _read_buf.reserve(size);
//    uv_buf_t uvbuf;
//    uvbuf.base = ptr;
//    uvbuf.len  = size;
//    retain();
//    uv_fs_read(_uvr.loop, &_uvr, _file, &uvbuf, 1, offset, uvx_on_read_complete);
//}
//
//void FSRequest::_write (int64_t offset, fn callback) {
//    set_busy();
//    _callback = callback;
//    auto nbufs = _bufs.size();
//    uv_buf_t uvbufs[nbufs];
//    uv_buf_t* ptr = uvbufs;
//    for (const auto& str : _bufs) {
//        ptr->base = const_cast<char*>(str.data()); // OK because libuv will only access bufs readonly
//        ptr->len  = str.length();
//        ++ptr;
//    }
//    retain();
//    uv_fs_write(_uvr.loop, &_uvr, _file, uvbufs, nbufs, offset, uvx_on_complete);
//}
//
//FSRequest::~FSRequest () {
//    assert(_state != State::BUSY);
//    cleanup();
//}
