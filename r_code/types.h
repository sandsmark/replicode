#ifndef	types_h
#define	types_h

#include	<cstddef>

#if defined(WIN32)
	#define	WINDOWS
	#define	ARCH_32
#elif defined(WIN64)
	#define	WINDOWS
	#define	ARCH_64
#elif defined(__GNUC__)
	#if __GNUC__ == 4
		#if __GNUC_MINOR__ < 3
//			#error "GNU C++ 4.3 or later is required to compile this program"
		#endif
	#endif

	#if defined(__x86_64)
		#define	ARCH_64
	#elif defined(__i386)
		#define	ARCH_32
	#endif
	#if defined(__linux)
		#define	LINUX
	#elif defined(__APPLE__)
		#define	OSX
	#endif
#endif

#if defined WINDOWS
	#define	WIN32_LEAN_AND_MEAN
	#define	_WIN32_WINNT	0x0501	//	i.e. win xp sp2
	#include	<windows.h>
	#include	<winsock2.h>
	#include	<hash_map>
	#define		UNORDERED_MAP	stdext::hash_map

	#if defined	BUILD_DLL
		#define dll	__declspec(dllexport)
	#else
		#define dll	__declspec(dllimport)
	#endif
	#define	dll_export	__declspec(dllexport)
	#define	dll_import	__declspec(dllimport)
	//#define cdecl		__cdecl		// no need to define this as the Windows do it anyway

	#pragma	warning(disable:	4530)	//	warning: exception disabled
	#pragma	warning(disable:	4996)	//	warning: this function may be unsafe
	#pragma	warning(disable:	4800)	//	warning: forcing value to bool
#elif defined LINUX || defined OSX
	#define dll
	#include	<iostream>
	#include	<string>
	#include	<pthread.h>
	#include	<semaphore.h>
	#include	<signal.h>
	#include	<limits.h>
	#include	<stdlib.h>
	#include	<vector>
	#include	<list>
	#if defined LINUX
		#include	<unordered_map>
		#include	<unordered_set>
		#define		UNORDERED_MAP	std::unordered_map
		#define		UNORDERED_SET	std::unordered_set
		#define		UNORDERED_MULTIMAP	std::unordered_multimap
		#define		UNORDERED_MULTISET	std::unordered_multiset
	#else
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define UNORDERED_MAP __gnu_cxx::hash_map
		#define		UNORDERED_SET	__gnu_cxx::hash_set
		#define		UNORDERED_MULTIMAP	__gnu_cxx::hash_multimap
		#define		UNORDERED_MULTISET	__gnu_cxx::hash_multiset
	#endif

	#define dll_export __attribute((visibility("default")))
	#define dll_import __attribute((visibility("default")))
	#define cdecl

	#include	<sys/socket.h>
	#include	<netinet/in.h>
	#include	<netinet/ip.h>
	#include	<arpa/inet.h>
	#define	SOCKET_ERROR	-1
	#define	INVALID_SOCKET	-1
	#define	closesocket(X)	close(X)
#else
	#error "This is a new platform"
#endif

#define	NEWLINE	'\n'

#define	WORD32_MASK					0xFFFFFFFF

#if defined	ARCH_32

	typedef	long						word32;
	typedef	short						word16;

	typedef	char						int8;
	typedef	unsigned	char			uint8;
	typedef	short						int16;
	typedef	unsigned	short			uint16;
	typedef	long						int32;
	typedef	unsigned	long			uint32;
	typedef	long		long			int64;
	typedef	unsigned	long	long	uint64;
	typedef	float						float32;
	typedef	double						float64;

	typedef	word32						word;
	typedef	word16						half_word;

	#define	HALF_WORD_SHIFT				16
	#define	HALF_WORD_HIGH_MASK			0xFFFF0000
	#define	HALF_WORD_LOW_MASK			0x0000FFFF
	#define	WORD_MASK					0xFFFFFFFF

#elif defined	ARCH_64

	typedef	int							word32;
	typedef	long						word64;

	typedef	char						int8;
	typedef	unsigned	char			uint8;
	typedef	short						int16;
	typedef	unsigned	short			uint16;
	typedef	int							int32;
	typedef	unsigned	int				uint32;
	typedef	long						int64;
	typedef	unsigned	long			uint64;
	typedef	float						float32;
	typedef	double						float64;

	typedef	word64						word;
	typedef	word32						half_word;

	#define	HALF_WORD_SHIFT				32
	#define	HALF_WORD_HIGH_MASK			0xFFFFFFFF00000000
	#define	HALF_WORD_LOW_MASK			0x00000000FFFFFFFF
	#define	WORD_MASK					0xFFFFFFFFFFFFFFFF

#endif

namespace	r_code{

#if defined	WINDOWS
	typedef	HINSTANCE						shared_object;
	typedef	HANDLE							thread;
	#define thread_ret						uint32
	#define thread_ret_val(ret)					return ret;
	typedef	LPTHREAD_START_ROUTINE			thread_function;
	#define	thread_function_call			WINAPI
	typedef	SOCKET							socket;
	typedef	HANDLE							semaphore;
	typedef	HANDLE							mutex;
	typedef	CRITICAL_SECTION				critical_section;
	typedef	HANDLE							timer;
	#define	signal_handler_function_call	WINAPI
	typedef	PHANDLER_ROUTINE				signal_handler;
#elif defined	LINUX
	typedef void *							shared_object;
	typedef pthread_t						thread;
	#define thread_ret						void *
	#define thread_ret_val(ret)					pthread_exit((thread_ret)ret);
	typedef thread_ret (*thread_function)(void *);
	#define thread_function_call
	typedef int								socket;
	typedef struct sockaddr					SOCKADDR;
	typedef	sem_t							semaphore;
	typedef pthread_mutex_t					mutex;
	typedef pthread_mutex_t					critical_section;
	typedef timer_t							timer;
	#define signal_handler_function_call
	typedef sighandler_t					signal_handler;
	#define stricmp strcasecmp
#elif defined	OSX
	#define stricmp strcasecmp
#endif
}


#endif
