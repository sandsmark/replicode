#ifndef	time_core_h
#define	time_core_h

#include	"utils.h"
#include	"time_job.h"


using	namespace	core;

namespace	r_exec{

	class	Mem;

	
	class	TimeCore:
	public	Thread{
	private:
		Mem	*mem;
		Timer	timer;
		bool	delegated;
	public:
		static	thread_ret thread_function_call	Run(void	*args);
		TimeCore(Mem	*i);
		~TimeCore();
	};
}


#endif