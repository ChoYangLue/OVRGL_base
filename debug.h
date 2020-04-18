#pragma once

#define	LOG(fmt, ...){	char temp[256]; snprintf(temp, sizeof(temp), fmt, __VA_ARGS__);	OutputDebugString(temp); }
#define	LOGLN(fmt, ...){ char temp[256]; snprintf(temp, sizeof(temp), fmt"\n", __VA_ARGS__); OutputDebugString(temp); }

#define DEBUG
#ifdef  DEBUG

#define DBGLOG(fmt, ...) { char temp[256]; snprintf(temp, sizeof(temp), fmt"\n", __VA_ARGS__); OutputDebugString(temp); }

#else

#define DBGLOG(...) {}

#endif //  DEBUG
