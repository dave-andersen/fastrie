#ifndef TICKER_H
#define TICKER_H

#if (defined(__MACH__) && defined(__APPLE__))
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

#ifdef _WIN32
#include<windows.h>
typedef unsigned __int64 uint64_t;
#else 
#include <stdint.h>
#include <errno.h>
#endif

uint64_t getTimeMilliseconds(void);
uint64_t getTimeHighRes(void);
uint64_t getTimerRes(void);
#endif // TICKER_H
