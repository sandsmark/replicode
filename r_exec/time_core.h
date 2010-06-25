#ifndef	time_core_h
#define	time_core_h

#include	"utils.h"
#include	"time_job.h"


using	namespace	core;

namespace	r_exec{

	class	_Mem;
	
	class	DelegatedCore:
	public	Thread{
	private:
		_Mem		*mem;
		Timer		timer;
		uint64		deadline;
		P<_TimeJob>	job;
	public:
		static	thread_ret thread_function_call	Wait(void	*args);
		DelegatedCore(_Mem	*m,uint64	deadline,_TimeJob	*j);
		~DelegatedCore();
	};

	class	r_exec_dll	TimeCore:
	public	Thread{
	private:
		_Mem	*mem;
	public:
		static	thread_ret thread_function_call	Run(void	*args);
		TimeCore(_Mem	*m);
		~TimeCore();
	};
}


#endif