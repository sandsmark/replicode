#include	"reduction_core.h"
#include	"mem.h"
#include	"init.h"


namespace	r_exec{

	thread_ret thread_function_call	ReductionCore::Run(void	*args){

		ReductionCore	*_this=((ReductionCore	*)args);

		while(1){

			ReductionJob	j=_this->mem->popReductionJob();
			if(j.overlay->is_alive()){

				uint64	now=Now();
				if(j.deadline>now)	//	in case of an input-less pgm or an |pgm, signaling jobs will inject productions if any.
					j.overlay->reduce(j.input,_this->mem);
			}
		}

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ReductionCore::ReductionCore(Mem	*i):Thread(),mem(i){
	}

	ReductionCore::~ReductionCore(){
	}
}