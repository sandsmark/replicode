#include	"time_core.h"
#include	"mem.h"


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

			uint64	now=Mem::Now();
			uint64	deadline=target-now;
			if(deadline>=0){	//	on time: spawn a delegate and wait for the due time

				TimeCore	*delegate_core=new	TimeCore(_this->mem);
				delegate_core->delegated=true;
				delegate_core->start(TimeCore::Run);

				_this->timer.start(deadline);
				_this->timer.wait();
			}else{	//	we are late: report

				std::cout<<"TimeCore report: late on target. Target was: "<<target<<" Actual performance time: "<<now<<std::endl;
			}
process:
			j.job->update(_this->mem);

			if(_this->delegated)
				thread_ret_val(0);
		}

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TimeCore::TimeCore(Mem	*i):Thread(),mem(i),delegated(false){
	}

	TimeCore::~TimeCore(){
	}
}