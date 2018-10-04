
#line 1 "SocksParser.rl"

#line 89 "SocksParser.rl"


#if defined(MACHINE_DATA)
#undef MACHINE_DATA

#line 11 "SocksParser.cc"
static const char _socks5_client_parser_eof_actions[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 0, 0, 0, 
	0, 0, 0
};

static const int socks5_client_parser_start = 1;
static const int socks5_client_parser_first_final = 13;
static const int socks5_client_parser_error = 0;

static const int socks5_client_parser_en_negotiate_reply = 9;
static const int socks5_client_parser_en_auth_reply = 11;
static const int socks5_client_parser_en_connect_reply = 1;


#line 95 "SocksParser.rl"

#endif

#if defined(MACHINE_INIT)
#undef MACHINE_INIT

#line 34 "SocksParser.cc"
	{
	cs = socks5_client_parser_start;
	}

#line 102 "SocksParser.rl"

#endif

#if defined(MACHINE_EXEC)
#undef MACHINE_EXEC

#line 46 "SocksParser.cc"
	{
	short _widec;
	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	switch ( cs ) {
case 1:
	if ( (*p) == 5 )
		goto tr1;
	goto tr0;
case 0:
	goto _out;
case 2:
	if ( (*p) == 0 )
		goto tr2;
	goto tr0;
case 3:
	if ( (*p) == 0 )
		goto tr3;
	goto tr0;
case 4:
	goto tr4;
case 5:
	_widec = (*p);
	if ( (*p) < 5 ) {
		if ( (*p) > 3 ) {
			if ( 4 <= (*p) && (*p) <= 4 ) {
				_widec = (short)(1152 + ((*p) - -128));
				if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
				if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 512;
			}
		} else {
			_widec = (short)(1152 + ((*p) - -128));
			if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
			if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 512;
		}
	} else if ( (*p) > 15 ) {
		if ( (*p) > 16 ) {
			if ( 17 <= (*p) )
 {				_widec = (short)(1152 + ((*p) - -128));
				if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
				if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 512;
			}
		} else if ( (*p) >= 16 ) {
			_widec = (short)(1152 + ((*p) - -128));
			if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
			if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 512;
		}
	} else {
		_widec = (short)(1152 + ((*p) - -128));
		if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
		if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 512;
	}
	switch( _widec ) {
		case 1540: goto tr6;
		case 1808: goto tr8;
		case 2052: goto tr10;
		case 2064: goto tr11;
	}
	if ( _widec < 1664 ) {
		if ( 1408 <= _widec && _widec <= 1663 )
			goto tr5;
	} else if ( _widec > 1919 ) {
		if ( 1920 <= _widec && _widec <= 2175 )
			goto tr9;
	} else
		goto tr7;
	goto tr0;
case 6:
	if ( (*p) == 2 )
		goto tr13;
	goto tr12;
case 13:
	goto tr24;
case 14:
	if ( (*p) == 2 )
		goto tr13;
	goto tr12;
case 7:
	_widec = (*p);
	if ( (*p) < 4 ) {
		if ( (*p) <= 3 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
		}
	} else if ( (*p) > 4 ) {
		if ( 5 <= (*p) )
 {			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
	}
	switch( _widec ) {
		case 258: goto tr13;
		case 516: goto tr15;
	}
	if ( _widec > 383 ) {
		if ( 384 <= _widec && _widec <= 639 )
			goto tr14;
	} else if ( _widec >= 128 )
		goto tr12;
	goto tr0;
case 15:
	_widec = (*p);
	if ( (*p) < 4 ) {
		if ( (*p) <= 3 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
		}
	} else if ( (*p) > 4 ) {
		if ( 5 <= (*p) )
 {			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 85 "SocksParser.rl"
atyp==0x01 ) _widec += 256;
	}
	switch( _widec ) {
		case 258: goto tr13;
		case 516: goto tr15;
	}
	if ( _widec > 383 ) {
		if ( 384 <= _widec && _widec <= 639 )
			goto tr14;
	} else if ( _widec >= 128 )
		goto tr12;
	goto tr24;
case 8:
	_widec = (*p);
	if ( (*p) < 16 ) {
		if ( (*p) <= 15 ) {
			_widec = (short)(640 + ((*p) - -128));
			if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 256;
		}
	} else if ( (*p) > 16 ) {
		if ( 17 <= (*p) )
 {			_widec = (short)(640 + ((*p) - -128));
			if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 256;
		}
	} else {
		_widec = (short)(640 + ((*p) - -128));
		if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 256;
	}
	switch( _widec ) {
		case 770: goto tr13;
		case 1040: goto tr17;
	}
	if ( _widec > 895 ) {
		if ( 896 <= _widec && _widec <= 1151 )
			goto tr16;
	} else if ( _widec >= 640 )
		goto tr12;
	goto tr0;
case 16:
	_widec = (*p);
	if ( (*p) < 16 ) {
		if ( (*p) <= 15 ) {
			_widec = (short)(640 + ((*p) - -128));
			if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 256;
		}
	} else if ( (*p) > 16 ) {
		if ( 17 <= (*p) )
 {			_widec = (short)(640 + ((*p) - -128));
			if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 256;
		}
	} else {
		_widec = (short)(640 + ((*p) - -128));
		if ( 
#line 86 "SocksParser.rl"
atyp==0x04 ) _widec += 256;
	}
	switch( _widec ) {
		case 770: goto tr13;
		case 1040: goto tr17;
	}
	if ( _widec > 895 ) {
		if ( 896 <= _widec && _widec <= 1151 )
			goto tr16;
	} else if ( _widec >= 640 )
		goto tr12;
	goto tr24;
case 9:
	if ( (*p) == 5 )
		goto tr18;
	goto tr0;
case 10:
	switch( (*p) ) {
		case -1: goto tr19;
		case 0: goto tr20;
		case 2: goto tr21;
	}
	goto tr0;
case 17:
	goto tr0;
case 11:
	if ( (*p) == 1 )
		goto tr22;
	goto tr0;
case 12:
	goto tr23;
case 18:
	goto tr24;
	}

	tr24: cs = 0; goto _again;
	tr0: cs = 0; goto f0;
	tr1: cs = 2; goto _again;
	tr2: cs = 3; goto f1;
	tr3: cs = 4; goto _again;
	tr4: cs = 5; goto f2;
	tr5: cs = 6; goto f3;
	tr7: cs = 6; goto f4;
	tr9: cs = 6; goto f5;
	tr6: cs = 7; goto f3;
	tr10: cs = 7; goto f5;
	tr8: cs = 8; goto f4;
	tr11: cs = 8; goto f5;
	tr18: cs = 10; goto _again;
	tr22: cs = 12; goto _again;
	tr12: cs = 13; goto f6;
	tr13: cs = 14; goto f6;
	tr14: cs = 14; goto f7;
	tr16: cs = 14; goto f8;
	tr15: cs = 15; goto f7;
	tr17: cs = 16; goto f8;
	tr19: cs = 17; goto f9;
	tr20: cs = 17; goto f10;
	tr21: cs = 17; goto f11;
	tr23: cs = 18; goto f12;

f6:
#line 18 "SocksParser.rl"
	{
        _EDEBUG("connect");
        if(rep) {
            do_error();
            {p++; goto _out; }
        }
        do_connected();
    }
	goto _again;
f3:
#line 48 "SocksParser.rl"
	{
        _EDEBUG("ip4");
    }
	goto _again;
f4:
#line 52 "SocksParser.rl"
	{
        _EDEBUG("ip6");
    }
	goto _again;
f2:
#line 56 "SocksParser.rl"
	{
        atyp = (uint8_t)*p;
        _EDEBUG("atyp: %d", atyp);
    }
	goto _again;
f1:
#line 61 "SocksParser.rl"
	{
        rep = (uint8_t)*p;
        _EDEBUG("rep: %d", rep);
    }
	goto _again;
f0:
#line 66 "SocksParser.rl"
	{
        do_error();
    }
	goto _again;
f12:
#line 27 "SocksParser.rl"
	{
        _EDEBUG("auth status");
        auth_status = (uint8_t)*p;
    }
#line 13 "SocksParser.rl"
	{
        _EDEBUG("auth");
        do_connect();
    }
	goto _again;
f10:
#line 32 "SocksParser.rl"
	{
        _EDEBUG("noauth method");
        noauth = true;
    }
#line 4 "SocksParser.rl"
	{ 
        _EDEBUG("negotiate");
        if(noauth) {
            do_connect();
        } else {
            do_auth();
        }
    }
	goto _again;
f11:
#line 37 "SocksParser.rl"
	{
        _EDEBUG("userpass method");
        noauth = false;
    }
#line 4 "SocksParser.rl"
	{ 
        _EDEBUG("negotiate");
        if(noauth) {
            do_connect();
        } else {
            do_auth();
        }
    }
	goto _again;
f9:
#line 42 "SocksParser.rl"
	{
        _EDEBUG("noacceptable method");
        do_error();
        {p++; goto _out; }
    }
#line 4 "SocksParser.rl"
	{ 
        _EDEBUG("negotiate");
        if(noauth) {
            do_connect();
        } else {
            do_auth();
        }
    }
	goto _again;
f7:
#line 48 "SocksParser.rl"
	{
        _EDEBUG("ip4");
    }
#line 18 "SocksParser.rl"
	{
        _EDEBUG("connect");
        if(rep) {
            do_error();
            {p++; goto _out; }
        }
        do_connected();
    }
	goto _again;
f5:
#line 48 "SocksParser.rl"
	{
        _EDEBUG("ip4");
    }
#line 52 "SocksParser.rl"
	{
        _EDEBUG("ip6");
    }
	goto _again;
f8:
#line 52 "SocksParser.rl"
	{
        _EDEBUG("ip6");
    }
#line 18 "SocksParser.rl"
	{
        _EDEBUG("connect");
        if(rep) {
            do_error();
            {p++; goto _out; }
        }
        do_connected();
    }
	goto _again;

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	switch ( _socks5_client_parser_eof_actions[cs] ) {
	case 1:
#line 66 "SocksParser.rl"
	{
        do_error();
    }
	break;
#line 484 "SocksParser.cc"
	}
	}

	_out: {}
	}

#line 109 "SocksParser.rl"

#endif

