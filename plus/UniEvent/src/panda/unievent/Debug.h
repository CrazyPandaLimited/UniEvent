#pragma once

// Library private debug macros

#define EVENT_LIB_DEBUG 0

#define _EDUMP(PTR, LEN, LIMIT) do { if(EVENT_LIB_DEBUG >= 1) if(LEN > 0 && LEN <= LIMIT) {fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); for(int i = 0; i < LEN; ++i) fprintf(stderr, "%02X ", (unsigned char)PTR[i]); fprintf(stderr, "\n");}} while(0)

#define _EDEBUGTHIS(fmt, ...) do { if(EVENT_LIB_DEBUG >= 1) fprintf(stderr, "%s:%d:%s(): [%p] " fmt "\n", __FILE__, __LINE__, __func__, this, ##__VA_ARGS__); } while (0)
#define _EDEBUG(fmt, ...) do { if (EVENT_LIB_DEBUG >= 1) fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define _ETRACE(fmt, ...) do { if (EVENT_LIB_DEBUG >= 2) fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define _ETRACETHIS(fmt, ...) do { if(EVENT_LIB_DEBUG >= 2) fprintf(stderr, "%s:%d:%s(): [%p] " fmt "\n", __FILE__, __LINE__, __func__, this, ##__VA_ARGS__); } while (0)

#define SHOW_PLACE fprintf(stderr, "FILE=%s, LINE=%d\n", __FILE__, __LINE__)
#define FORCE_PTR_EVAL(arg) fprintf(stderr, "%s=%x\n", #arg, arg)
