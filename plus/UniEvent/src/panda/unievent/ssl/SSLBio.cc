#include "SSLBio.h"
#include "../Stream.h"

using panda::string;
using namespace panda::unievent::ssl;

using membuf_t = SSLBio::membuf_t;

static int bio_new (BIO* bio) {
    membuf_t* b = new membuf_t();
    BIO_set_shutdown(bio, 1);
    BIO_set_init(bio, 1);
    BIO_set_mem_eof_return(bio, -1); //  =>  bio->num = -1
    BIO_set_data(bio, b);
    
    b->handle = nullptr;
    return 1;
}

static int bio_free (BIO* bio) {
    if (!bio) return 0;
    membuf_t* b = (membuf_t*)BIO_get_data(bio);

    _EDEBUG("(%p) CURLEN=%lu", bio, b->buf.length());
    
    if (!BIO_get_shutdown(bio) || !BIO_get_init(bio) || !b) return 1;
    delete b;
    BIO_set_data(bio, nullptr);
    return 1;
}

static int bio_write (BIO* bio, const char* in, int _inl) {
    size_t inl = _inl;
    membuf_t* b = (membuf_t*)BIO_get_data(bio);

    _EDEBUG("(%p) INLEN=%lu CURLEN=%lu", bio, inl, b->buf.length());
    
    BIO_clear_retry_flags(bio);

    string& buf = b->buf;
    auto buflen = buf.length();

    if (buf.capacity() < buflen + inl) {
        // TODO: buf_alloc may return empty string (if exception). As for now in that case, empty string will just allocate as necessary
        // but we should better somehow pass an ENOBUF error to upper level
        string newbuf = b->handle->buf_alloc(buflen + inl);
        if (buflen) newbuf += buf;
        buf = newbuf;
    }

    buf.append(in, inl);

    return inl;
}

static int bio_read (BIO* bio, char* out, int _outl) {
    int outl = _outl;
    membuf_t* b = (membuf_t*)BIO_get_data(bio);

    _EDEBUG("(%p) len=%d have=%lu", bio, outl, b->buf.length());
    if(outl == 5) _EDUMP(b->buf, 5, 5);

    BIO_clear_retry_flags(bio);
    string& buf = b->buf;
    if (int(buf.length()) < outl) outl = buf.length();
    if (outl > 0) {
        _EDEBUG("outl > 0: %d", outl);
        memcpy(out, buf.data(), outl);
        buf.offset(outl);
    } else if (!buf) {
        #if OPENSSL_VERSION_NUMBER < 0x10100000L
        outl = bio->num;
        #else
        outl = -1;
        #endif
        if (outl != 0) BIO_set_retry_read(bio);
    }
        
    _EDEBUG("return: %d", outl);
    return outl;
}

static int bio_puts (BIO* bio, const char* str) {
    _EDEBUG("(%p)", bio);
    
    return bio_write(bio, str, strlen(str));
}

static int bio_gets (BIO* bio, char* buf, int size) {
    _EDEBUG("(%p)", bio);
    
    membuf_t* b = (membuf_t*)BIO_get_data(bio);
    BIO_clear_retry_flags(bio);
    int j = b->buf.length();
    if (j > size - 1) j = size - 1;
    if (j <= 0) {
        if (size > 0) *buf = 0;
        return 0;
    }
    int i;
    const char* p = b->buf.data();
    for (i = 0; i < j; i++) if (p[i] == '\n') {
        i++;
        break;
    }
    /* i is now the max num of bytes to copy, either j or up to and including the first newline */
    i = bio_read(bio, buf, i);
    if (i > 0) buf[i] = '\0';
    return i;
}

static long bio_ctrl (BIO* bio, int cmd, long num, void* ptr) {
    _EDEBUG("(%p) cmd=%d num=%li ptr=%p", bio, cmd, num, ptr);

    long ret = 1;
    membuf_t* b = (membuf_t*)BIO_get_data(bio);
    switch (cmd) {
        case BIO_CTRL_RESET:
            _EDEBUG("BIO_CTRL_RESET");
            b->buf.clear();
            break;
        case BIO_CTRL_EOF:
            _EDEBUG("BIO_CTRL_EOF");
            ret = (long)(b->buf.length() == 0);
            break;
        case BIO_C_SET_BUF_MEM_EOF_RETURN:
            _EDEBUG("BIO_C_SET_BUF_MEM_EOF_RETURN");
            #if OPENSSL_VERSION_NUMBER < 0x10100000L
            bio->num = (int)num;             
            #else
            //ret = BIO_ctrl(bio, cmd, num, ptr);
	    //ret = BIO_set_mem_eof_return(bio, num);
	    ret = 0;
            #endif
            break;
        case BIO_CTRL_INFO:
            _EDEBUG("BIO_CTRL_INFO");
            ret = (long)b->buf.length();
            if (ptr != nullptr) *((char**)ptr) = b->buf.buf();
            break;
        case BIO_C_SET_BUF_MEM:
            _EDEBUG("BIO_C_SET_BUF_MEM");
            bio_free(bio);
            BIO_set_shutdown(bio, (int)num);
            BIO_set_data(bio, ptr);
            break;
        case BIO_C_GET_BUF_MEM_PTR:
            _EDEBUG("BIO_C_GET_BUF_MEM_PTR");
            if (ptr != nullptr) *((void**)ptr) = b;
            break;
        case BIO_CTRL_GET_CLOSE:
            _EDEBUG("BIO_CTRL_GET_CLOSE");
            ret = BIO_get_shutdown(bio);
            break;
        case BIO_CTRL_SET_CLOSE:
            _EDEBUG("BIO_CTRL_SET_CLOSE");
            BIO_set_shutdown(bio, (int)num);
            break;
        case BIO_CTRL_WPENDING:
            _EDEBUG("BIO_CTRL_WPENDING");
            ret = 0;
            break;
        case BIO_CTRL_PENDING:
            _EDEBUG("BIO_CTRL_PENDING");
            ret = (long)b->buf.length();
            break;
        case BIO_CTRL_DUP:
        case BIO_CTRL_FLUSH:
            ret = 1;
            _EDEBUG("BIO_CTRL_FLUSH");
            break;
        case BIO_CTRL_PUSH:
        case BIO_CTRL_POP:
        default:
            _EDEBUG("BIO_CTRL_DEFAULT");
            ret = 0;
            break;
    }
    return ret;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L

BIO_METHOD SSLBio::_method = {
    BIO_TYPE_MEM, "memory buffer", bio_write, bio_read, bio_puts, bio_gets, bio_ctrl, bio_new, bio_free, nullptr
};


#else

BIO_METHOD* SSLBio::_method() {
    _EDEBUG();
    
    thread_local BIO_METHOD *bio = nullptr;
    if (bio) {
        return bio;
    }
    bio = BIO_meth_new(BIO_TYPE_MEM, "memory buffer");
    BIO_meth_set_write(bio, bio_write);
    BIO_meth_set_read(bio, bio_read); 
    BIO_meth_set_puts(bio, bio_puts);
    BIO_meth_set_gets(bio, bio_gets);
    BIO_meth_set_ctrl(bio, bio_ctrl); 
    BIO_meth_set_create(bio, bio_new); 
    BIO_meth_set_destroy(bio, bio_free);
    BIO_meth_set_callback_ctrl(bio, nullptr);
            
    return bio;
}

#endif
