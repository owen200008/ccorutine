#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#ifdef __GNUC__
#define CCTHREADID_DWORD    pthread_t
#define CCORUTINETLS_Key	pthread_key_t
#define CCLockfreequeueLikely(x) __builtin_expect((x), true)
#define CCLockfreequeueUnLikely(x) __builtin_expect((x), false)
#elif defined(_MSC_VER)
#include <Windows.h>
#define CCTHREADID_DWORD    DWORD
#define CCORUTINETLS_Key	DWORD
#define CCLockfreequeueLikely(x) x
#define CCLockfreequeueUnLikely(x) x
#endif


#ifndef USE_EXTRA_MEMORYALLOC
#define USE_EXTRA_MEMORYALLOC
#define CCORUTINE_MALLOC    malloc
#define CCORUTINE_FREE      free
#endif

#ifndef CCOROUTINE_LOGGER
#define CCOROUTINE_LOGGER printf
#endif

#ifndef CCOROUTINE_VECTOR
#define CCOROUTINE_VECTOR std::vector
#endif

#ifndef ccsnprintf
#ifdef __GNUC__
#define ccsnprintf snprintf
#else
#define ccsnprintf sprintf_s
#endif
#endif


