#define OSX
#if defined(OSX) || defined(LINUX)
	#include <pthread.h>
	#include <sys/time.h>
	typedef pthread_t						thread;
	#define thread_ret						void *
	#define thread_ret_val(ret)					pthread_exit((thread_ret)ret);
	typedef thread_ret (*thread_function)(void *);
	#define thread_function_call

	typedef pthread_mutex_t						mutex;
#endif
