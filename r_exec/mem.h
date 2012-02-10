//	mem.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef	mem_h
#define	mem_h

#include	"reduction_core.h"
#include	"time_core.h"
#include	"pgm_overlay.h"
#include	"binding_map.h"
#include	"dll.h"

#include	<list>

#include	"../r_comp/segments.h"

#include	"../../CoreLibrary/trunk/CoreLibrary/pipe.h"


namespace	r_exec{

	// The rMem.
	// Maintains 2 pipes of jobs (injection, update, etc.). each job is processed asynchronously by instances of ReductionCore and TimeCore.
	// Pipes and threads are created at starting time and deleted at stopping time.
	// Groups and IPGMControllers are cleared up when only held by jobs;
	// 	- when a group is not projected anywhere anymore, it is invalidated (it releases all its views) and when a job attempts an update, the latter is cancelled.
	// 	- when a reduction core attempts to perform a reduction for an ipgm-controller that is not projected anywhere anymore, the reduction is cancelled.
	// In addition:
	// 	- when an object is scheduled for injection and the target group does not exist anymore (or is invalidated), the injection is cancelled.
	// 	- when an object is scheduled for propagation of sln changes and has no view anymore, the operation is cancelled.
	// Main processing in _Mem::update().
	class	r_exec_dll	_Mem:
	public	r_code::Mem{
	public:
		typedef	enum{
			NOT_STARTED=0,
			RUNNING=1,
			STOPPED=2
		}State;
	protected:
		// Parameters::Init.
		uint32	base_period;
		uint32	reduction_core_count;
		uint32	time_core_count;

		// Parameters::System.
		float32	mdl_inertia_sr_thr;
		uint32	mdl_inertia_cnt_thr;
		float32	tpx_dsr_thr;
		uint32	min_sim_time_horizon;
		uint32	max_sim_time_horizon;
		float32	sim_time_horizon;
		uint32	tpx_time_horizon;
		uint32	perf_sampling_period;
		float32	float_tolerance;
		uint32	time_tolerance;

		// Parameters::Debug.
		bool	debug;
		uint32	ntf_mk_res;
		uint32	goal_pred_success_res;

		// Parameters::Run.
		uint32	probe_level;

		PipeNN<P<_ReductionJob>,1024>	*reduction_job_queue;
		PipeNN<P<TimeJob>,1024>			*time_job_queue;
		ReductionCore					**reduction_cores;
		TimeCore						**time_cores;

		// Performance stats.
		uint32	reduction_job_count;
		uint64	reduction_job_avg_latency;	// latency: popping time.-pushing time; the lower the better.
		uint64	_reduction_job_avg_latency;	// previous value.
		uint32	time_job_count;
		uint64	time_job_avg_latency;		// latency: deadline-the time the job is popped from the pipe; if <0, not registered (as it is too late for action); the higher the better.
		uint64	_time_job_avg_latency;		// previous value.

		CriticalSection	time_jobCS;
		CriticalSection	reduction_jobCS;

		uint32			core_count;
		CriticalSection	core_countCS;
		
		State			state;
		CriticalSection	stateCS;
		Semaphore		*stop_sem;	// blocks the rMem until all cores terminate.

		P<Group>	_root;	// holds everything.
		Code		*_stdin;
		Code		*_stdout;
		Code		*_self;

		void	reset();	// clear the content of the mem.

		CriticalSection	object_registerCS;
		CriticalSection	objectsCS;

		// Utilities.
		class	GroupState{
		public:
			float32	former_sln_thr;
			bool	was_c_active;
			bool	is_c_active;
			bool	was_c_salient;
			bool	is_c_salient;
			GroupState(	float32	former_sln_thr,
						bool	was_c_active,
						bool	is_c_active,
						bool	was_c_salient,
						bool	is_c_salient):former_sln_thr(former_sln_thr),was_c_active(was_c_active),is_c_active(is_c_active),was_c_salient(was_c_salient),is_c_salient(is_c_salient){}
		};

		void	_update_visibility(Group	*group,GroupState	*state,View	*view);
		void	_update_saliency(Group	*group,GroupState	*state,View	*view);
		void	_update_activation(Group	*group,GroupState	*state,View	*view);
		void	_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr);
		void	_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path);
		void	_propagate_sln(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path);

		std::vector<Group	*>	initial_groups;	// convenience; cleared after start();

		uint32	last_oid;
		uint64	starting_time;

		virtual	void	init_timings(uint64	now)=0;

		_Mem();

		void	_unpack_code(Code	*hlp,uint16	fact_object_index,Code	*fact_object,uint16	read_index)	const;
	public:
		static	_Mem	*Get(){	return	(_Mem	*)Mem::Get();	}

		typedef	enum{
			STDIN=0,
			STDOUT=1
		}STDGroupID;

		virtual	~_Mem();

		void	init(uint32	base_period,
					uint32	reduction_core_count,
					uint32	time_core_count,
					float32	mdl_inertia_sr_thr,
					uint32	mdl_inertia_cnt_thr,
					float32	tpx_dsr_thr,
					uint32	min_sim_time_horizon,
					uint32	max_sim_time_horizon,
					float32	sim_time_horizon,
					uint32	tpx_time_horizon,
					uint32	perf_sampling_period,
					float32	float_tolerance,
					uint32	time_tolerance,
					bool	debug,
					uint32	ntf_mk_res,
					uint32	goal_pred_success_res,
					uint32	probe_level);

		uint64	get_base_period()						const{	return	base_period;	}
		uint64	get_probe_level()						const{	return	probe_level;	}
		uint64	get_starting_time()						const{	return	starting_time;	}
		float32	get_float_tolerance()					const{	return	float_tolerance;	}
		uint32	get_time_tolerance()					const{	return	time_tolerance;	}
		float32	get_mdl_inertia_sr_thr()				const{	return	mdl_inertia_sr_thr;	}
		uint32	get_mdl_inertia_cnt_thr()				const{	return	mdl_inertia_cnt_thr;	}
		float32	get_tpx_dsr_thr()						const{	return	tpx_dsr_thr;	}
		uint32	get_min_sim_time_horizon()				const{	return	min_sim_time_horizon;	}
		uint32	get_max_sim_time_horizon()				const{	return	max_sim_time_horizon;	}
		uint64	get_sim_time_horizon(uint64	horizon)	const{	return	horizon*sim_time_horizon;	}
		uint32	get_tpx_time_horizon()					const{	return	tpx_time_horizon;	}
		
		bool	get_debug()								const{	return	debug;	}
		uint32	get_ntf_mk_res()						const{	return	debug?ntf_mk_res:1;	}
		uint32	get_goal_pred_success_res(Group	*host,uint64	time_to_live)	const{
			
			if(debug)
				return	goal_pred_success_res;
			if(time_to_live=0)
				return	1;
			uint64	base=base_period*host->get_upr();
			return	Utils::GetResilience(time_to_live,base);	
		}

		Code	*get_root()		const;
		Code	*get_stdin()	const;
		Code	*get_stdout()	const;
		Code	*get_self()		const;

		State	check_state();			// called by delegates after waiting in case stop() is called in the meantime.
		void	start_core();			// called upon creation of a delegate.
		void	shutdown_core();		// called upon completion of a delegate's task.

		uint64	start();	// return the starting time.
		void	stop();		// after stop() the content is cleared and one has to call load() and start() again.

		uint32	get_oid();

		// Called by groups at update time.
		// Called by PGMOverlays at reduction time.
		// Called by AntiPGMOverlays at signaling time and reduction time.
		virtual	void	inject_notification(View	*view,bool	lock)=0;


		// Internal core processing	////////////////////////////////////////////////////////////////

		_ReductionJob	*popReductionJob();
		void			pushReductionJob(_ReductionJob	*j);
		TimeJob			*popTimeJob();
		void			pushTimeJob(TimeJob	*j);

		// Called at each update period.
		// - set the final resilience value, if 0, delete.
		// - set the final saliency.
		// - set the final activation.
		// - set the final visibility, cov.
		// - propagate saliency changes.
		// - inject next update job for the group.
		// - inject new signaling jobs if act pgm with no input or act |pgm.
		// - notify high and low values.
		void	update(Group	*group);

		// Called upon successful reduction.
		virtual	void	inject_new_object(View	*view)=0;
				void	inject_existing_object(View	*view,Code	*object,Group	*host,bool	lock);

		// Called as a result of a group update (sln change).
		// Calls mod_sln on the object's view with morphed sln changes.
		void	propagate_sln(Code	*object,float32	change,float32	source_sln_thr);

		// Called by groups.
		void	inject_copy(View	*view,Group	*destination,uint64	now);	// for cov; NB: no cov for groups, r-groups, models, pgm or notifications.
		void	inject_reduction_jobs(View	*view,Group	*host);				// builds reduction jobs from host's inputs and own overlay (assuming host is c-salient and the view is salient);
																			// builds reduction jobs from host's inputs and viewing groups' overlays (assuming host is c-salient and the view is salient).
		// Called by cores.
		void	register_reduction_job_latency(uint64	latency);
		void	register_time_job_latency(uint64	latency);
		void	inject_perf_stats();

		// Interface for overlays and I/O devices ////////////////////////////////////////////////////////////////
		virtual	void	inject(View	*view)=0;
		virtual	Code	*check_existence(Code	*object)=0;	// returns the existing object if any, or object otherwise: in the latter case, packing may occur.

		// rMem to rMem.
		// The view must contain the destination group (either stdin or stdout) as its grp member.
		// To be redefined by object transport aware subcalsses.
		virtual	void	eject(View	*view,uint16	nodeID);

		// From rMem to I/O device.
		// To be redefined by object transport aware subcalsses.
		virtual	void	eject(Code	*command);

		virtual	r_code::Code	*_build_object(Atom	head)	const=0;
		virtual	r_code::Code	*build_object(Atom	head)	const=0;
		
		// unpacking of high-level patterns: upon loading or reception.
		void	unpack_hlp(Code	*hlp)	const;
		Code	*unpack_fact(Code	*hlp,uint16	fact_index)	const;
		Code	*unpack_fact_object(Code	*hlp,uint16	fact_object_index)	const;

		// packing of high-level patterns: upon dynamic generation or transmission.
		void	pack_hlp(Code	*hlp)	const;
		void	pack_fact(Code	*fact,Code	*hlp,uint16	&write_index,std::vector<P<Code>	>	*references)	const;
		void	pack_fact_object(Code	*fact_object,Code	*hlp,uint16	&write_index,std::vector<P<Code>	>	*references)	const;

		Code	*clone(Code	*original)	const;	// deep cloning.
	};

	// O is the class of the objects held by the rMem (except groups and notifications):
	// 	r_exec::LObject if non distributed, or
	// 	RObject (see the integration project) when network-aware.
	// Notification objects and groups are instances of r_exec::LObject (they are not network-aware).
	// Objects are built at reduction time as r_exec:LObjects and packed into instances of O when O is network-aware.
	// Shared resources:
	// 	Mem::object_register: accessed by Mem::update, Mem::injectNow and Mem::deleteObject (see above).
	template<class	O>	class	Mem:
	public	_Mem{
	protected:
		std::list<Code	*>											objects;			// to insert in an image (getImage()); in order of injection.
		UNORDERED_SET<O	*,typename	O::Hash,typename	O::Equal>	object_register;	// to eliminate duplicates (content-wise); does not include groups.

		template<class	_O>	void	bind(View	*view,uint64	t){

			Utils::SetTimestamp<View>(view,VIEW_IJT,t);
			_O	*object=(_O	*)view->object;
			object->views.insert(view);
			object->bind(this);
			objectsCS.enter();
			object->position_in_objects=objects.insert(objects.end(),object);
			objectsCS.leave();
			object->is_registered=true;
		}

		void	init_timings(uint64	now);

		// Functions called by internal processing of jobs (see internal processing section below).
		void	inject_new_object(View	*view);	// also called by inject() (see below).
	public:
		Mem();
		virtual	~Mem();

		// Called at load time.
		r_code::Code	*build_object(r_code::SysObject	*source)	const;

		// Called at runtime.
		r_code::Code	*_build_object(Atom	head)	const;
		r_code::Code	*build_object(Atom	head)	const;

		bool	load(std::vector<r_code::Code	*>	*objects,
					uint32							stdin_oid,
					uint32							stdout_oid,
					uint32							self_oid);	// call before start; no mod/set/eje will be executed (only inj);
																// ijt will be set at now=Time::Get() whatever the source code.
																// return false on error.
		void	delete_object(Code	*object);	// called by object destructors/Group::clear().

		// External device I/O	////////////////////////////////////////////////////////////////
		r_comp::Image	*get_image();	// create an image; fill with all objects; call only when stopped.

		// Executive device functions	////////////////////////////////////////////////////////
		
		// Called by the reduction core.
		void	inject(View	*view);
		Code	*check_existence(Code	*object);

		// Called by the communication device (I/O).
		void	inject(O	*object,View	*view);

		// Variant of injectNow optimized for notifications.
		void	inject_notification(View	*view,bool	lock);

		// Called by time cores.	////////////////////////////////////////////////////////////////
		void	update(SaliencyPropagationJob	*j);
	};

	r_exec_dll r_exec::Mem<r_exec::LObject> *Run(const	char	*user_operator_library_path,
												uint64			(*time_base)(),
												const	char	*seed_path,
												const	char	*source_file_name);
}


#include	"mem.tpl.cpp"


#endif