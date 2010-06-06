#include	"time_job.h"
#include	"mem.h"


namespace	r_exec{

	bool	_TimeJob::is_alive()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////

	TimeJob::TimeJob(){
	}

	TimeJob::TimeJob(_TimeJob	*j,uint64	ijt):target_time(ijt){

		job=j;
	}

	bool	TimeJob::is_alive()	const{

		return	job->is_alive();
	}

	////////////////////////////////////////////////////////////

	UpdateJob::UpdateJob(Group	*g){

		group=g;
	}

	void	UpdateJob::update(Mem	*m){

		m->update(this);
	}

	////////////////////////////////////////////////////////////

	SignalingJob::SignalingJob(IPGMController	*o){

		controller=o;
	}

	bool	SignalingJob::is_alive()	const{

		return	controller->is_alive();
	}

	////////////////////////////////////////////////////////////

	AntiPGMSignalingJob::AntiPGMSignalingJob(IPGMController	*o):SignalingJob(o){
	}

	void	AntiPGMSignalingJob::update(Mem	*m){

		m->update(this);
	}

	////////////////////////////////////////////////////////////

	InputLessPGMSignalingJob::InputLessPGMSignalingJob(IPGMController	*o):SignalingJob(o){
	}

	void	InputLessPGMSignalingJob::update(Mem	*m){

		m->update(this);
	}

	////////////////////////////////////////////////////////////

	InjectionJob::InjectionJob(View	*v){

		view=v;
	}

	void	InjectionJob::update(Mem	*m){

		m->update(this);
	}

	////////////////////////////////////////////////////////////

	SaliencyPropagationJob::SaliencyPropagationJob(Object	*o,float32	sln_change,float32	source_sln_thr):sln_change(sln_change),source_sln_thr(source_sln_thr){

		object=o;
	}
	
	void	SaliencyPropagationJob::update(Mem	*m){

		m->update(this);
	}
}