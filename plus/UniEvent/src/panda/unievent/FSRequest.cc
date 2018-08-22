#include <panda/unievent/FSRequest.h>
#include <panda/unievent/global.h>
using namespace panda::unievent;

#define PE_FS_CALL_SYNC_NOERR(process_code) \
    uv_fs_t _uvr;                           \
    _uvr.loop = nullptr;                       \
    process_code                            \
    uv_fs_req_cleanup(&_uvr);

#define PE_FS_CALL_SYNC(call_code, get_result_code)             \
    PE_FS_CALL_SYNC_NOERR({                                     \
        call_code                                               \
        if (_uvr.result < 0) throw FSRequestError(_uvr.result); \
        get_result_code                                         \
    })

file_t FSRequest::open (string_view path, int flags, int mode) {
    file_t ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_open(_uvr.loop, &_uvr, path_str, flags, mode, nullptr);
    }, {
        ret = (file_t)_uvr.result;
    });
    return ret;
}

void FSRequest::close (file_t file) {
    PE_FS_CALL_SYNC({
        uv_fs_close(_uvr.loop, &_uvr, file, nullptr);
    }, {});
}

stat_t FSRequest::stat (string_view path) {
    stat_t ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_stat(_uvr.loop, &_uvr, path_str, nullptr);
    }, {
        ret = _uvr.statbuf;
    });
    return ret;
}

stat_t FSRequest::stat (file_t file) {
    stat_t ret;
    PE_FS_CALL_SYNC({
        uv_fs_fstat(_uvr.loop, &_uvr, file, nullptr);
    }, {
        ret = _uvr.statbuf;
    });
    return ret;
}

stat_t FSRequest::lstat (string_view path) {
    stat_t ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_lstat(_uvr.loop, &_uvr, path_str, nullptr);
    }, {
        ret = _uvr.statbuf;
    });
    return ret;
}

void FSRequest::sync (file_t file) {
    PE_FS_CALL_SYNC({
        uv_fs_fsync(_uvr.loop, &_uvr, file, nullptr);
    }, {});
}

void FSRequest::datasync (file_t file) {
    PE_FS_CALL_SYNC({
        uv_fs_fdatasync(_uvr.loop, &_uvr, file, nullptr);
    }, {});
}

void FSRequest::truncate (file_t file, int64_t offset) {
    PE_FS_CALL_SYNC({
        uv_fs_ftruncate(_uvr.loop, &_uvr, file, offset, nullptr);
    }, {});
}

void FSRequest::chmod (string_view path, int mode) {
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_chmod(_uvr.loop, &_uvr, path_str, mode, nullptr);
    }, {});
}

void FSRequest::chmod (file_t file, int mode) {
    PE_FS_CALL_SYNC({
        uv_fs_fchmod(_uvr.loop, &_uvr, file, mode, nullptr);
    }, {});
}

void FSRequest::utime (string_view path, double atime, double mtime) {
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_utime(_uvr.loop, &_uvr, path_str, atime, mtime, nullptr);
    }, {});
}

void FSRequest::utime (file_t file, double atime, double mtime) {
    PE_FS_CALL_SYNC({
        uv_fs_futime(_uvr.loop, &_uvr, file, atime, mtime, nullptr);
    }, {});
}

void FSRequest::chown (string_view path, uid_t uid, gid_t gid) {
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_chown(_uvr.loop, &_uvr, path_str, uid, gid, nullptr);
    }, {});
}

void FSRequest::chown (file_t file, uid_t uid, gid_t gid) {
    PE_FS_CALL_SYNC({
        uv_fs_fchown(_uvr.loop, &_uvr, file, uid, gid, nullptr);
    }, {});
}

string FSRequest::read (file_t file, size_t length, int64_t offset) {
    string ret;
    char* ptr = ret.reserve(length);
    uv_buf_t uvbuf;
    uvbuf.base = ptr;
    uvbuf.len  = length;
    PE_FS_CALL_SYNC({
        uv_fs_read(_uvr.loop, &_uvr, file, &uvbuf, 1, offset, nullptr);
    }, {
        ret.length(length);
    });
    return ret;
}

void FSRequest::_write (file_t file, uv_buf_t* uvbufs, size_t nbufs, int64_t offset) {
    PE_FS_CALL_SYNC({
        uv_fs_write(_uvr.loop, &_uvr, file, uvbufs, nbufs, offset, nullptr);
    }, {});
}

void FSRequest::unlink (string_view path) {
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_unlink(_uvr.loop, &_uvr, path_str, nullptr);
    }, {});
}

void FSRequest::mkdir (string_view path, int mode) {
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_mkdir(_uvr.loop, &_uvr, path_str, mode, nullptr);
    }, {});
}

void FSRequest::rmdir (string_view path) {
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_rmdir(_uvr.loop, &_uvr, path_str, nullptr);
    }, {});
}

DirEntries FSRequest::scandir (string_view path, int flags) {
    DirEntries ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_scandir(_uvr.loop, &_uvr, path_str, flags, nullptr);
    }, {
        size_t nent = (size_t)_uvr.result;
        if (nent) {
            ret.reserve(nent);
            auto uv_entries = (uv_dirent_t**)_uvr.ptr;
            for (size_t i = 0; i < nent; ++i) ret.emplace(ret.cend(), string(uv_entries[i]->name), (DirEntry::Type)uv_entries[i]->type);
        }
    });
    return ret;
}

void FSRequest::rename (string_view path, string_view new_path) {
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    PE_FS_CALL_SYNC({
        uv_fs_rename(_uvr.loop, &_uvr, path_str, new_path_str, nullptr);
    }, {});
}

size_t FSRequest::sendfile (file_t out_fd, file_t in_fd, int64_t in_offset, size_t length) {
    size_t ret;
    PE_FS_CALL_SYNC({
        uv_fs_sendfile(_uvr.loop, &_uvr, out_fd, in_fd, in_offset, length, nullptr);
    }, {
        ret = (size_t)_uvr.result;
    });
    return ret;
}

void FSRequest::link (string_view path, string_view new_path) {
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    PE_FS_CALL_SYNC({
        uv_fs_link(_uvr.loop, &_uvr, path_str, new_path_str, nullptr);
    }, {});
}

void FSRequest::symlink (string_view path, string_view new_path, SymlinkFlags flags) {
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    PE_FS_CALL_SYNC({
        uv_fs_symlink(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, nullptr);
    }, {});
}

string FSRequest::readlink (string_view path) {
    string ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_readlink(_uvr.loop, &_uvr, path_str, nullptr);
    }, {
        ret.assign((const char*)_uvr.ptr, (size_t)_uvr.result); // _uvr.ptr is not null-terminated
    });
    return ret;
}

string FSRequest::realpath (string_view path) {
    string ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_realpath(_uvr.loop, &_uvr, path_str, nullptr);
    }, {
        ret.assign((const char*)_uvr.ptr); // _uvr.ptr is null-terminated
    });
    return ret;
}

void FSRequest::copyfile (string_view path, string_view new_path, CopyFileFlags flags) {
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    PE_FS_CALL_SYNC({
        uv_fs_copyfile(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, nullptr);
    }, {});
}

bool FSRequest::access (string_view path, int mode) {
    bool ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC_NOERR({
        uv_fs_access(_uvr.loop, &_uvr, path_str, mode, nullptr);
        switch (_uvr.result) {
            case 0:
                ret = true;
                break;
            case UV_EACCES:
            case UV_EROFS:
                ret = false;
                break;
            default:
                throw RequestError(_uvr.result);
        }
    });
    return ret;
}

string FSRequest::mkdtemp (string_view path) {
    string ret;
    PEXS_NULL_TERMINATE(path, path_str);
    PE_FS_CALL_SYNC({
        uv_fs_mkdtemp(_uvr.loop, &_uvr, path_str, nullptr);
    }, {
        ret.assign(_uvr.path);
    });
    return ret;
}


void FSRequest::uvx_on_open_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    if (!err) req->_file = req->_uvr.result;
    req->set_complete();
    req->_open_callback(req, err, req->_file);
    req->release();
}

void FSRequest::open (string_view path, int flags, int mode, open_fn callback) {
    set_busy();
    _open_callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_open(_uvr.loop, &_uvr, path_str, flags, mode, uvx_on_open_complete);
}

void FSRequest::uvx_on_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    req->set_complete();
    req->_callback(req, err);
    req->release();
}

void FSRequest::close (fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_close(_uvr.loop, &_uvr, _file, uvx_on_complete);
}

void FSRequest::uvx_on_stat_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    stat_t val;
    if (!err) val = req->_uvr.statbuf;
    req->set_complete();
    req->_stat_callback(req, err, val);
    req->release();
}

void FSRequest::stat (string_view path, stat_fn callback) {
    set_busy();
    _stat_callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_stat(_uvr.loop, &_uvr, path_str, uvx_on_stat_complete);
}

void FSRequest::stat (stat_fn callback) {
    set_busy();
    _stat_callback = callback;
    uv_fs_fstat(_uvr.loop, &_uvr, _file, uvx_on_stat_complete);
}

void FSRequest::lstat (string_view path, stat_fn callback) {
    set_busy();
    _stat_callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_lstat(_uvr.loop, &_uvr, path_str, uvx_on_stat_complete);
}

void FSRequest::sync (fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_fsync(_uvr.loop, &_uvr, _file, uvx_on_complete);
}

void FSRequest::datasync (fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_fdatasync(_uvr.loop, &_uvr, _file, uvx_on_complete);
}

void FSRequest::truncate (int64_t offset, fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_ftruncate(_uvr.loop, &_uvr, _file, offset, uvx_on_complete);
}

void FSRequest::chmod (string_view path, int mode, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_chmod(_uvr.loop, &_uvr, path_str, mode, uvx_on_complete);
}

void FSRequest::chmod (int mode, fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_fchmod(_uvr.loop, &_uvr, _file, mode, uvx_on_complete);
}

void FSRequest::utime (string_view path, double atime, double mtime, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_utime(_uvr.loop, &_uvr, path_str, atime, mtime, uvx_on_complete);
}

void FSRequest::utime (double atime, double mtime, fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_futime(_uvr.loop, &_uvr, _file, atime, mtime, uvx_on_complete);
}

void FSRequest::chown (string_view path, uid_t uid, gid_t gid, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_chown(_uvr.loop, &_uvr, path_str, uid, gid, uvx_on_complete);
}

void FSRequest::chown (uid_t uid, gid_t gid, fn callback) {
    set_busy();
    _callback = callback;
    uv_fs_fchown(_uvr.loop, &_uvr, _file, uid, gid, uvx_on_complete);
}

void FSRequest::unlink (string_view path, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_unlink(_uvr.loop, &_uvr, path_str, uvx_on_complete);
}

void FSRequest::mkdir (string_view path, int mode, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_mkdir(_uvr.loop, &_uvr, path_str, mode, uvx_on_complete);
}

void FSRequest::rmdir (string_view path, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_rmdir(_uvr.loop, &_uvr, path_str, uvx_on_complete);
}

void FSRequest::uvx_on_scandir_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    DirEntries ret;
    if (!err) {
        size_t nent = (size_t)req->_uvr.result;
        if (nent) {
            ret.reserve(nent);
            auto uv_entries = (uv_dirent_t**)req->_uvr.ptr;
            for (size_t i = 0; i < nent; ++i) ret.emplace(ret.cend(), string(uv_entries[i]->name), (DirEntry::Type)uv_entries[i]->type);
        }
    }
    req->set_complete();
    req->_scandir_callback(req, err, ret);
    req->release();
}

void FSRequest::scandir (string_view path, int flags, scandir_fn callback) {
    set_busy();
    _scandir_callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_scandir(_uvr.loop, &_uvr, path_str, flags, uvx_on_scandir_complete);
}

void FSRequest::rename (string_view path, string_view new_path, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    uv_fs_rename(_uvr.loop, &_uvr, path_str, new_path_str, uvx_on_complete);
}

void FSRequest::uvx_on_sendfile_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    size_t ret = 0;
    if (!err) ret = (size_t)req->_uvr.result;
    req->set_complete();
    req->_sendfile_callback(req, err, ret);
    req->release();
}

void FSRequest::sendfile (file_t out_fd, file_t in_fd, int64_t in_offset, size_t length, sendfile_fn callback) {
    set_busy();
    _sendfile_callback = callback;
    uv_fs_sendfile(_uvr.loop, &_uvr, out_fd, in_fd, in_offset, length, uvx_on_sendfile_complete);
}

void FSRequest::link (string_view path, string_view new_path, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    uv_fs_link(_uvr.loop, &_uvr, path_str, new_path_str, uvx_on_complete);
}

void FSRequest::symlink (string_view path, string_view new_path, SymlinkFlags flags, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    uv_fs_symlink(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, uvx_on_complete);
}

void FSRequest::uvx_on_readlink_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    string ret;
    if (!err) ret.assign((const char*)req->_uvr.ptr, (size_t)req->_uvr.result); // _uvr.ptr is not null-terminated
    req->set_complete();
    req->_read_callback(req, err, ret);
    req->release();
}

void FSRequest::readlink (string_view path, read_fn callback) {
    set_busy();
    _read_callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_readlink(_uvr.loop, &_uvr, path_str, uvx_on_readlink_complete);
}

void FSRequest::uvx_on_realpath_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    string ret;
    if (!err) ret.assign((const char*)req->_uvr.ptr); // _uvr.ptr is null-terminated
    req->set_complete();
    req->_read_callback(req, err, ret);
    req->release();
}

void FSRequest::realpath (string_view path, read_fn callback) {
    set_busy();
    _read_callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    uv_fs_realpath(_uvr.loop, &_uvr, path_str, uvx_on_realpath_complete);
}

void FSRequest::copyfile (string_view path, string_view new_path, CopyFileFlags flags, fn callback) {
    set_busy();
    _callback = callback;
    PEXS_NULL_TERMINATE(path, path_str);
    PEXS_NULL_TERMINATE(new_path, new_path_str);
    uv_fs_copyfile(_uvr.loop, &_uvr, path_str, new_path_str, (int)flags, uvx_on_complete);
}

void FSRequest::uvx_on_read_complete (uv_fs_t* uvreq) {
    auto req = rcast<FSRequest*>(uvreq);
    RequestError err(req->_uvr.result > 0 ? 0 : req->_uvr.result);
    if (!err) req->_read_buf.length((size_t)req->_uvr.result);
    req->set_complete();
    req->_read_callback(req, err, req->_read_buf);
    req->_read_buf.clear();
    req->release();
}

void FSRequest::read (size_t size, int64_t offset, read_fn callback) {
    set_busy();
    _read_callback = callback;
    _read_buf.clear();
    char* ptr = _read_buf.reserve(size);
    uv_buf_t uvbuf;
    uvbuf.base = ptr;
    uvbuf.len  = size;
    uv_fs_read(_uvr.loop, &_uvr, _file, &uvbuf, 1, offset, uvx_on_read_complete);
}

void FSRequest::_write (int64_t offset, fn callback) {
    set_busy();
    _callback = callback;
    auto nbufs = _bufs.size();
    uv_buf_t uvbufs[nbufs];
    uv_buf_t* ptr = uvbufs;
    for (const auto& str : _bufs) {
        ptr->base = const_cast<char*>(str.data()); // OK because libuv will only access bufs readonly
        ptr->len  = str.length();
        ++ptr;
    }
    uv_fs_write(_uvr.loop, &_uvr, _file, uvbufs, nbufs, offset, uvx_on_complete);
}

FSRequest::~FSRequest () {
    assert(_state != State::BUSY);
    cleanup();
}
