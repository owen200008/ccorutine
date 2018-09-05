#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#ifdef __GNUC__
#if defined(__APPLE__)
typedef unsigned long DWORD;
#else
typedef unsigned long DWORD;
#endif
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>
#define CCSleep(x) usleep(x)
#elif defined(_MSC_VER)
#include <Windows.h>
#define CCSleep(x) ::Sleep(x)
#endif

#ifdef _DEBUG
#define TIMES_FAST  500000
#define POW2SIZE    524288
#else
#define TIMES_FAST  2000000
#define POW2SIZE    2097152
#endif


