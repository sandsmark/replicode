#include	"mem.h"/*
#include	"init.h"
#include	"replicode_defs.h"
#include	"operator.h"
#include	"group.h"
#include	"../r_code/utils.h"
#include	<math.h>
*/

namespace	r_exec{

	_Mem::_Mem():state(NOT_STARTED){
	}

	_Mem::~_Mem(){

		if(state==STARTED)
			reset();
		root=NULL;
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

		for(uint32	i=0;i<delegates.size();++i){

			Thread::TerminateAndWait(delegates[i]);
			delete	delegates[i];
		}
		delegates.clear();

		delete	object_register_sem;
		delete	objects_sem;
		delete	state_sem;

		delete	reduction_job_queue;
		delete	time_job_queue;
	}

	_Mem::State	_Mem::get_state(){

		state_sem->acquire();
		State	s=state;
		state_sem->release();

		return	s;
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::add_delegate(uint64	dealine,_TimeJob	*j){

		state_sem->acquire();
		if(state==STARTED)
			delegates.push_back(new	DelegatedCore(this,dealine,j));
		state_sem->release();
	}

	void	_Mem::stop(){

		state_sem->acquire();
		state=STOPPED;
		state_sem->release();

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			Thread::TerminateAndWait(reduction_cores[i]);
		for(i=0;i<time_core_count;++i)
			Thread::TerminateAndWait(time_cores[i]);

		reset();
	}

	////////////////////////////////////////////////////////////////

	ReductionJob	_Mem::popReductionJob(){

		return	reduction_job_queue->pop();
	}

	void	_Mem::pushReductionJob(ReductionJob	j){

		reduction_job_queue->push(j);
	}

	TimeJob	_Mem::popTimeJob(){

		return	time_job_queue->pop();
	}

	void	_Mem::pushTimeJob(TimeJob	j){

		time_job_queue->push(j);
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