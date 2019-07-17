#pragma once
#include <cstdio>

// to use inside the library

#ifndef EVENT_LIB_DEBUG
#define EVENT_LIB_DEBUG 0
#endif

#if EVENT_LIB_DEBUG >= 3
#define _ANSI_COLOR_RED "\x1b[31m"
#define _ANSI_COLOR_GREEN "\x1b[32m"
#define _ANSI_COLOR_YELLOW "\x1b[33m"
#define _ANSI_COLOR_BLUE "\x1b[34m"
#define _ANSI_COLOR_MAGENTA "\x1b[35m"
#define _ANSI_COLOR_CYAN "\x1b[36m"
#define _ANSI_COLOR_RESET "\x1b[0m"
#else
#define _ANSI_COLOR_RED ""
#define _ANSI_COLOR_GREEN ""
#define _ANSI_COLOR_YELLOW ""
#define _ANSI_COLOR_BLUE ""
#define _ANSI_COLOR_MAGENTA ""
#define _ANSI_COLOR_CYAN ""
#define _ANSI_COLOR_RESET ""
#endif

#define _EDUMP(PTR, LEN, LIMIT) do { if(EVENT_LIB_DEBUG >= 1) if(LEN > 0 && LEN <= LIMIT) {fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); for(int i = 0; i < LEN; ++i) fprintf(stderr, "%02X ", (unsigned char)PTR[i]); fprintf(stderr, "\n");}} while(0)

#define _EDEBUGTHIS(fmt, ...) do { if(EVENT_LIB_DEBUG >= 1) fprintf(stderr, "%s:%d:%s(): [%p] " fmt "\n", __FILE__, __LINE__, __func__, this, ##__VA_ARGS__); } while (0)
#define _EDEBUG(fmt, ...) do { if (EVENT_LIB_DEBUG >= 1) fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define _ETRACE(fmt, ...) do { if (EVENT_LIB_DEBUG >= 2) fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define _ETRACETHIS(fmt, ...) do { if(EVENT_LIB_DEBUG >= 2) fprintf(stderr, "%s:%d:%s(): [%p] " fmt "\n", __FILE__, __LINE__, __func__, this, ##__VA_ARGS__); } while (0)
#define _ECTOR() do { if(EVENT_LIB_DEBUG >= 2) fprintf(stderr, "%s:%d:%s(): [%p] " _ANSI_COLOR_RED "[ctor]" _ANSI_COLOR_RESET "\n", __FILE__, __LINE__, __func__, this); } while (0)
#define _EDTOR() do { if(EVENT_LIB_DEBUG >= 2) fprintf(stderr, "%s:%d:%s(): [%p] " _ANSI_COLOR_GREEN "[dtor]" _ANSI_COLOR_RESET "\n", __FILE__, __LINE__, __func__, this); } while (0)

#define SHOW_PLACE fprintf(stderr, "FILE=%s, LINE=%d\n", __FILE__, __LINE__)
#define FORCE_PTR_EVAL(arg) fprintf(stderr, "%s=%x\n", #arg, arg)
