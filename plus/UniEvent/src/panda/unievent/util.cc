#include "util.h"
#include "Tcp.h"
#include "Udp.h"
#include "Pipe.h"
#include "Resolver.h"
#include <uv.h>

using panda::net::SockAddr;

namespace panda { namespace unievent {

AddrInfo sync_resolve (backend::Backend* be, string_view host, uint16_t port, const AddrInfoHints& hints, bool use_cache) {
    auto l = SyncLoop::get(be);
    AddrInfo ai;
    l->resolver()->resolve()->node(string(host))->port(port)->hints(hints)->use_cache(use_cache)->on_resolve([&ai](const AddrInfo& res, const CodeError& err, const Resolver::RequestSP) {
        if (err) throw err;
        ai = res;
    })->run();
    l->run();
    return ai;
}

void setsockopt (fd_t sock, int level, int optname, const void* optval, int optlen) {
    #ifdef _WIN32
        if (::setsockopt(sock, level, optname, (const char*)optval, optlen)) throw last_sys_code_error();
    #else
        if (::setsockopt(sock, level, optname, optval, (socklen_t)optlen)) throw last_sys_code_error();
    #endif
}

string hostname () {
    string ret(20);
    size_t len = ret.capacity();
    int err = uv_os_gethostname(ret.buf(), &len);
    if (err) {
        if (err != UV_ENOBUFS) throw uvx_code_error(err);
        ret.reserve(len);
        err = uv_os_gethostname(ret.buf(), &len);
        if (err) throw uvx_code_error(err);
    }
    ret.length(len);
    return ret;
}

size_t get_rss () {
    size_t rss;
    int err = uv_resident_set_memory(&rss);
    if (err) throw uvx_code_error(err);
    return rss;
}

uint64_t get_free_memory  () {
    return uv_get_free_memory();
}

uint64_t get_total_memory () {
    return uv_get_total_memory();
}

std::vector<InterfaceAddress> interface_info () {
    uv_interface_address_t* uvlist;
    int cnt;
    int err = uv_interface_addresses(&uvlist, &cnt);
    if (err) throw uvx_code_error(err);

    std::vector<InterfaceAddress> ret;
    ret.reserve(cnt);
    for (int i = 0; i < cnt; ++i) {
        auto& uvrow = uvlist[i];
        InterfaceAddress row;
        row.name = uvrow.name;
        std::memcpy(row.phys_addr, uvrow.phys_addr, sizeof(uvrow.phys_addr));
        row.is_internal = uvrow.is_internal;
        row.address = SockAddr((sockaddr*)&uvrow.address);
        row.netmask = SockAddr((sockaddr*)&uvrow.netmask);
        ret.push_back(row);
    }

    uv_free_interface_addresses(uvlist, cnt);

    return ret;
}

std::vector<CpuInfo> cpu_info () {
    uv_cpu_info_t* uvlist;
    int cnt;
    int err = uv_cpu_info(&uvlist, &cnt);
    if (err) throw uvx_code_error(err);

    std::vector<CpuInfo> ret;
    ret.reserve(cnt);
    for (int i = 0; i < cnt; ++i) {
        auto& uvrow = uvlist[i];
        CpuInfo row;
        row.model = uvrow.model;
        row.speed = uvrow.speed;
        row.cpu_times.user = uvrow.cpu_times.user;
        row.cpu_times.nice = uvrow.cpu_times.nice;
        row.cpu_times.sys  = uvrow.cpu_times.sys;
        row.cpu_times.idle = uvrow.cpu_times.idle;
        row.cpu_times.irq  = uvrow.cpu_times.irq;
        ret.push_back(row);
    }

    uv_free_cpu_info(uvlist, cnt);

    return ret;
}

ResourceUsage get_rusage () {
    uv_rusage_t d;
    int err = uv_getrusage(&d);
    if (err) throw uvx_code_error(err);

    ResourceUsage ret;
    ret.utime.sec  = d.ru_utime.tv_sec;
    ret.utime.usec = d.ru_utime.tv_usec;
    ret.stime.sec  = d.ru_stime.tv_sec;
    ret.stime.usec = d.ru_stime.tv_usec;
    ret.maxrss     = d.ru_maxrss;
    ret.ixrss      = d.ru_ixrss;
    ret.idrss      = d.ru_idrss;
    ret.isrss      = d.ru_isrss;
    ret.minflt     = d.ru_minflt;
    ret.majflt     = d.ru_majflt;
    ret.nswap      = d.ru_nswap;
    ret.inblock    = d.ru_inblock;
    ret.oublock    = d.ru_oublock;
    ret.msgsnd     = d.ru_msgsnd;
    ret.msgrcv     = d.ru_msgrcv;
    ret.nsignals   = d.ru_nsignals;
    ret.nvcsw      = d.ru_nvcsw;
    ret.nivcsw     = d.ru_nivcsw;

    return ret;
}

const HandleType& guess_type (file_t file) {
    auto uvt = uv_guess_handle(file);
    switch (uvt) {
        //case UV_TTY       : return TTY::TYPE;
        //case UV_FILE      : ???;
        case UV_NAMED_PIPE: return Pipe::TYPE;
        case UV_UDP       : return Udp::TYPE;
        case UV_TCP       : return Tcp::TYPE;
        default           : return Handle::UNKNOWN_TYPE;
    }
}

CodeError sys_code_error (int syserr) {
    // that appears to be a bug in gcc (well, in libstdc++); per 1.9.5.1p4 it should map POSIX error codes to generic_category
    #ifdef _WIN32
        return std::error_code(syserr, std::system_category());
    #else
        return std::error_code(syserr, std::generic_category());
    #endif
}

CodeError last_sys_code_error () {
    #ifdef _WIN32
        return sys_code_error(WSAGetLastError());
    #else
        return sys_code_error(errno);
    #endif
}

CodeError uvx_code_error (int uverr) {
    assert(uverr);
    switch (uverr) {
        case UV_E2BIG          : return CodeError(std::errc::argument_list_too_long);
        case UV_EACCES         : return CodeError(std::errc::permission_denied);
        case UV_EADDRINUSE     : return CodeError(std::errc::address_in_use);
        case UV_EADDRNOTAVAIL  : return CodeError(std::errc::address_not_available);
        case UV_EAFNOSUPPORT   : return CodeError(std::errc::address_family_not_supported);
        case UV_EAGAIN         : return CodeError(std::errc::resource_unavailable_try_again);
        case UV_EAI_ADDRFAMILY : return CodeError(errc::ai_address_family_not_supported);
        case UV_EAI_AGAIN      : return CodeError(errc::ai_temporary_failure);
        case UV_EAI_BADFLAGS   : return CodeError(errc::ai_bad_flags);
        case UV_EAI_BADHINTS   : return CodeError(errc::ai_bad_hints);
        case UV_EAI_CANCELED   : return CodeError(errc::ai_request_canceled);
        case UV_EAI_FAIL       : return CodeError(errc::ai_permanent_failure);
        case UV_EAI_FAMILY     : return CodeError(errc::ai_family_not_supported);
        case UV_EAI_MEMORY     : return CodeError(errc::ai_out_of_memory);
        case UV_EAI_NODATA     : return CodeError(errc::ai_no_address);
        case UV_EAI_NONAME     : return CodeError(errc::ai_unknown_node_or_service);
        case UV_EAI_OVERFLOW   : return CodeError(errc::ai_argument_buffer_overflow);
        case UV_EAI_PROTOCOL   : return CodeError(errc::ai_resolved_protocol_unknown);
        case UV_EAI_SERVICE    : return CodeError(errc::ai_service_not_available_for_socket_type);
        case UV_EAI_SOCKTYPE   : return CodeError(errc::ai_socket_type_not_supported);
        case UV_EALREADY       : return CodeError(std::errc::connection_already_in_progress);
        case UV_EBADF          : return CodeError(std::errc::bad_file_descriptor);
        case UV_EBUSY          : return CodeError(std::errc::device_or_resource_busy);
        case UV_ECANCELED      : return CodeError(std::errc::operation_canceled);
        case UV_ECHARSET       : return CodeError(errc::invalid_unicode_character);
        case UV_ECONNABORTED   : return CodeError(std::errc::connection_aborted);
        case UV_ECONNREFUSED   : return CodeError(std::errc::connection_refused);
        case UV_ECONNRESET     : return CodeError(std::errc::connection_reset);
        case UV_EDESTADDRREQ   : return CodeError(std::errc::destination_address_required);
        case UV_EEXIST         : return CodeError(std::errc::file_exists);
        case UV_EFAULT         : return CodeError(std::errc::bad_address);
        case UV_EFBIG          : return CodeError(std::errc::file_too_large);
        case UV_EHOSTUNREACH   : return CodeError(std::errc::host_unreachable);
        case UV_EINTR          : return CodeError(std::errc::interrupted);
        case UV_EINVAL         : return CodeError(std::errc::invalid_argument);
        case UV_EIO            : return CodeError(std::errc::io_error);
        case UV_EISCONN        : return CodeError(std::errc::already_connected);
        case UV_EISDIR         : return CodeError(std::errc::is_a_directory);
        case UV_ELOOP          : return CodeError(std::errc::too_many_symbolic_link_levels);
        case UV_EMFILE         : return CodeError(std::errc::too_many_files_open);
        case UV_EMSGSIZE       : return CodeError(std::errc::message_size);
        case UV_ENAMETOOLONG   : return CodeError(std::errc::filename_too_long);
        case UV_ENETDOWN       : return CodeError(std::errc::network_down);
        case UV_ENETUNREACH    : return CodeError(std::errc::network_unreachable);
        case UV_ENFILE         : return CodeError(std::errc::too_many_files_open_in_system);
        case UV_ENOBUFS        : return CodeError(std::errc::no_buffer_space);
        case UV_ENODEV         : return CodeError(std::errc::no_such_device);
        case UV_ENOENT         : return CodeError(std::errc::no_such_file_or_directory);
        case UV_ENOMEM         : return CodeError(std::errc::not_enough_memory);
        case UV_ENONET         : return CodeError(errc::not_on_network);
        case UV_ENOPROTOOPT    : return CodeError(std::errc::no_protocol_option);
        case UV_ENOSPC         : return CodeError(std::errc::no_space_on_device);
        case UV_ENOSYS         : return CodeError(std::errc::function_not_supported);
        case UV_ENOTCONN       : return CodeError(std::errc::not_connected);
        case UV_ENOTDIR        : return CodeError(std::errc::not_a_directory);
        case UV_ENOTEMPTY      : return CodeError(std::errc::directory_not_empty);
        case UV_ENOTSOCK       : return CodeError(std::errc::not_a_socket);
        case UV_ENOTSUP        : return CodeError(std::errc::not_supported);
        case UV_EPERM          : return CodeError(std::errc::operation_not_permitted);
        case UV_EPIPE          : return CodeError(std::errc::broken_pipe);
        case UV_EPROTO         : return CodeError(std::errc::protocol_error);
        case UV_EPROTONOSUPPORT: return CodeError(std::errc::protocol_not_supported);
        case UV_EPROTOTYPE     : return CodeError(std::errc::wrong_protocol_type);
        case UV_ERANGE         : return CodeError(std::errc::result_out_of_range);
        case UV_EROFS          : return CodeError(std::errc::read_only_file_system);
        case UV_ESHUTDOWN      : return CodeError(errc::transport_endpoint_shutdown);
        case UV_ESPIPE         : return CodeError(std::errc::invalid_seek);
        case UV_ESRCH          : return CodeError(std::errc::no_such_process);
        case UV_ETIMEDOUT      : return CodeError(std::errc::timed_out);
        case UV_ETXTBSY        : return CodeError(std::errc::text_file_busy);
        case UV_EXDEV          : return CodeError(std::errc::cross_device_link);
        case UV_UNKNOWN        : return CodeError(errc::unknown_error);
        case UV_ENXIO          : return CodeError(std::errc::no_such_device_or_address);
        case UV_EMLINK         : return CodeError(std::errc::too_many_links);
        case UV_EHOSTDOWN      : return CodeError(errc::host_down);
        case UV_EREMOTEIO      : return CodeError(errc::remote_io);
        case UV_ENOTTY         : return CodeError(std::errc::inappropriate_io_control_operation);
        default                : return CodeError(errc::unknown_error);
    }
}

}}
