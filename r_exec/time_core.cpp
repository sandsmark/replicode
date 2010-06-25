#include	"time_core.h"
#include	"mem.h"
#include	"init.h"


namespace	r_exec{

	thread_ret thread_function_call	TimeCore::Run(void	*args){

		TimeCore	*_this=((TimeCore	*)args);

		while(1){

			TimeJob	j=_this->mem->popTimeJob();
			if(!j.is_alive())
				continue;

			uint64	target=j.target_time;
			if(target==0)	//	0 means ASAP.
				goto	process;

			uint64	now=Now();
			int64	deadline=target-now;
			if(deadline>=0){	//	on time: spawn a delegate to wait for the due time; delegate will die when done.

				_this->mem->add_delegate(deadline,j.job);
				continue;
			}else{	//	we are late: report.

				std::cout<<"TimeCore report: late on target. Target was: "<<target<<" lag: "<<-deadline/1000<<" ms"<<std::endl;
			}
process:
			j.job->update(_this->mem);
		}

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TimeCore::TimeCore(_Mem	*m):Thread(),mem(m){
	}

	TimeCore::~TimeCore(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	thread_ret thread_function_call	DelegatedCore::Wait(void	*args){

		DelegatedCore	*_this=((DelegatedCore	*)args);

		_this->timer.start(_this->deadline);
		_this->timer.wait();
		
		_this->job->update(_this->mem);

		//delete	_this;	//	remove from mem::delegates.
		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DelegatedCore::DelegatedCore(_Mem	*m,uint64	deadline,_TimeJob	*j):Thread(),mem(m),deadline(deadline),job(j){

		start(DelegatedCore::Wait);
	}

	DelegatedCore::~DelegatedCore(){
	}
}