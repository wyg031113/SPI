#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
/*调试信息*/
#define DEBUG_ON
#define _DEBUG_IF_ 1
#define DEBUG_LOG_ON

#ifdef DEBUG_ON
#define DEBUGMSG(format, ...) do {if (_DEBUG_IF_) { fprintf(stderr, "%s(%d)-%s: ",__FILE__, __LINE__, __FUNCTION__); fprintf(stderr, format, __VA_ARGS__); } }while (0)
#else
#define DEBUGMSG(x)
#endif

#define CHECK(x) do{ if((x)==-1) { fprintf(stderr, "%s(%s-%d):%s\n", __FILE__, __FUNCTION__, __LINE__, strerror(errno)); exit(-1);} }while(0)
#define CHECK2(x) do{ if((x)!=0) { fprintf(stderr, "%s(%s-%d):failed!\n", __FILE__, __FUNCTION__, __LINE__); exit(-1);} }while(0)
#define CHECKMSG(x, msg) do{ if((x)!=0) { fprintf(stderr, "%s(%s-%d):%s\n", __FILE__, __FUNCTION__, __LINE__, msg); exit(-1);} }while(0)
#define ASSERT(x) do{ if(!(x)) { fprintf(stderr, "ASSERT Failed:%s(%s-%d):%s\n", __FILE__, __FUNCTION__, __LINE__, #x); exit(-1);} }while(0)
#define err_quit(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); fprintf(stderr, "\n"); exit(-1); }while (0)

#endif /*__DEBUG_H__*/
