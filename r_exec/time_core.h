#ifndef	time_core_h
#define	time_core_h

#include	"utils.h"
#include	"time_job.h"


using	namespace	core;

namespace	r_exec{

	class	_Mem;

	class	r_exec_dll	TimeCore:
	public	Thread{
	private:
		_Mem	*mem;
		Timer	timer;
		bool	delegated;
	public:
		static	thread_ret thread_function_call	Run(void	*args);
		TimeCore(_Mem	*m);
		~TimeCore();
	};
}


#endif