#include	"mem.h"/*
#include	"init.h"
#include	"replicode_defs.h"
#include	"operator.h"
#include	"group.h"
#include	"../r_code/utils.h"
#include	<math.h>
*/

namespace	r_exec{

	_Mem::_Mem(){
	}

	_Mem::~_Mem(){

		reset();
	}

	void	_Mem::init(uint32	base_period,	//	in us; same for upr, spr and res.
						uint32	reduction_core_count,
						uint32	time_core_count){

		this->base_period=base_period;
		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;
	}

	void	_Mem::reset(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		root=NULL;
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::stop(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			Thread::TerminateAndWait(reduction_cores[i]);
		for(i=0;i<time_core_count;++i)
			Thread::TerminateAndWait(time_cores[i]);

		reset();
	}
	
	void	_Mem::suspend(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->suspend();
		for(i=0;i<time_core_count;++i)
			time_cores[i]->suspend();
	}
	
	void	_Mem::resume(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->resume();
		for(i=0;i<time_core_count;++i)
			time_cores[i]->resume();
	}

	////////////////////////////////////////////////////////////////

	ReductionJob	_Mem::popReductionJob(){

		return	reduction_job_queue.pop();
	}

	void	_Mem::pushReductionJob(ReductionJob	j){

		reduction_job_queue.push(j);
	}

	TimeJob	_Mem::popTimeJob(){

		return	time_job_queue.pop();
	}

	void	_Mem::pushTimeJob(TimeJob	j){

		time_job_queue.push(j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::update(UpdateJob	*j){

		update((Group	*)j->group);
	}

	void	_Mem::update(AntiPGMSignalingJob	*j){

		j->controller->signal_anti_pgm(this);
	}

	void	_Mem::update(InputLessPGMSignalingJob	*j){

		j->controller->signal_input_less_pgm(this);
	}

	void	_Mem::update(InjectionJob	*j){

		injectNow(j->view);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(LObject	*object,uint16	nodeID){
	}
}