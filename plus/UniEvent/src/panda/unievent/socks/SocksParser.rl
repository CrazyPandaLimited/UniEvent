%%{
    machine socks5_client_parser;

    action negotiate { 
        _EDEBUG("negotiate");
        if(noauth) {
            do_connect();
        } else {
            do_auth();
        }
    }
    
    action auth {
        _EDEBUG("auth");
        do_connect();
    }
    
    action connect {
        _EDEBUG("connect");
        if(rep) {
            do_error();
            fbreak;
        }
        do_connected();
    }
    
    action auth_status {
        _EDEBUG("auth status");
        auth_status = (uint8_t)*fpc;
    }

    action noauth_auth_method {
        _EDEBUG("noauth method");
        noauth = true;
    }
    
    action userpass_auth_method {
        _EDEBUG("userpass method");
        noauth = false;
    }

    action noacceptable_auth_method {
        _EDEBUG("noacceptable method");
        do_error();
        fbreak;
    }

    action ip4 {
        _EDEBUG("ip4");
    }
    
    action ip6 {
        _EDEBUG("ip6");
    }

    action atyp {
        atyp = (uint8_t)*fpc;
        _EDEBUG("atyp: %d", atyp);
    }
    
    action rep {
        rep = (uint8_t)*fpc;
        _EDEBUG("rep: %d", rep);
    }
    
    action error {
        do_error();
    }
    
    ver=0x05;
    byte=any;

    auth_method = 0x00 @noauth_auth_method | 0x02 @userpass_auth_method | 0xFF @noacceptable_auth_method;
    negotiate_reply := (ver auth_method) @negotiate $!error;   

    auth_ver = 0x01;
    auth_status = any @auth_status;
    auth_reply := (auth_ver auth_status) @auth @!error;

    rep = 0x00;
    rsv = 0x00;
    atyp = byte @atyp;
    dst_addr_ip4 = 4*byte $ip4;
    dst_addr_ip6 = 16*byte $ip6;
    dst_addr =  dst_addr_ip4 when {atyp==0x01} | 
                dst_addr_ip6 when {atyp==0x04};
    dst_port = 2*byte;
    connect_reply := (ver rep @rep rsv atyp dst_addr dst_port) @connect @!error;   
}%%

#if defined(MACHINE_DATA)
#undef MACHINE_DATA
%%{
    write data;
}%%
#endif

#if defined(MACHINE_INIT)
#undef MACHINE_INIT
%%{
    write init;
}%%
#endif

#if defined(MACHINE_EXEC)
#undef MACHINE_EXEC
%%{
    write exec;
}%%
#endif

