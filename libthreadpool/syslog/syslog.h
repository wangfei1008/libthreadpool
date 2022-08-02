#ifndef C_LANGUAGE_SYSTEMLOG_20111214_WANGFIE_H
#define C_LANGUAGE_SYSTEMLOG_20111214_WANGFIE_H

#ifdef  WX_API_DLLEXP
#define WX_API_DLLEXP extern "C" __declspec(dllimport)
#else
#define WX_API_DLLEXP extern "C" __declspec(dllexport)
#endif

//#define __FUNCTION__ "function no name"

#define MAX_LOG_BUFFER 10240

#define LOG_NOLOG 0xff

#define LOG_NOERROR 0x01

#define LOG_NODEBUG 0x02

#define LOG_NOINFO 0x04

#define LOG_NOPRINTF 0x08

#define LOG_INFO  LOG_NOINFO
#define LOG_ERROR LOG_NOERROR
#define LOG_DEBUG LOG_NODEBUG

//#define  logwrite TRACE

#define logerror(fmt, ...)            \
 	logwrite(LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt ,__VA_ARGS__)

#define logdebug(fmt, ...) \
 	logwrite(LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt ,__VA_ARGS__)

#define loginfo(fmt, ...) \
	logwrite(LOG_INFO, __FILE__, __LINE__, __FUNCTION__, fmt ,__VA_ARGS__)

#define logrelease  logclose()

WX_API_DLLEXP int logopen();
WX_API_DLLEXP int logwrite(int level, const char* filename, const int line, const char* funname, const char* fmt, ...);
WX_API_DLLEXP void logclose();
WX_API_DLLEXP void logsetparm(char type);

#endif//C_LANGUAGE_SYSTEMLOG_20111214_WANGFIE_H
