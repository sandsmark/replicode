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

	//	The rMem.
	//	Maintains 2 pipes of jobs (injection, update, etc.). each job is processed asynchronously by instances of ReductionCore and TimeCore.
	//	Pipes and threads are created at starting time and deleted at stopping time.
	//	Groups and IPGMControllers are cleared up when only held by jobs;
	//		- when a group is not projected anywhere anymore, it is invalidated (it releses all its views) and when a job attempts an update, the latter is cancelled.
	//		- when a reduction core attempts to perform a reduction for an ipgm-controller that is not projected anywhere anymore, the reduction is cancelled.
	//	In addition:
	//		- when an object is scheduled for injection and the target group does not exist anymore (or is invalidated), the injection is cancelled.
	//		- when an object is scheduled for propagation of sln changes and has no view anymore, the operation is cancelled.
	//	Main processing in _Mem::update().
	class	r_exec_dll	_Mem:
	public	r_code::Mem{
	public:
		typedef	enum{
			D_RUNNING=0,
			D_RUNNING_AFTER_SUSPENSION=1,
			D_STOPPED=2,
			D_STOPPED_AFTER_SUSPENSION=3
		}DState;	//	status for delegates.
	protected:
		uint32	base_period;
		uint32	ntf_mk_res;
		uint32	goal_res;
		uint32	asmp_res;
		uint32	sim_res;
		float32	float_tolerance;
		uint32	time_tolerance;
		uint64	goal_record_resilience;

		PipeNN<P<_ReductionJob>,1024>	*reduction_job_queue;
		PipeNN<P<TimeJob>,1024>			*time_job_queue;
		
		uint32			reduction_core_count;
		ReductionCore	**reduction_cores;
		uint32			time_core_count;
		TimeCore		**time_cores;

		uint32			core_count;
		CriticalSection	core_countCS;

		typedef	enum{
			NOT_STARTED=0,
			RUNNING=1,
			SUSPENDED=2,
			STOPPED=3
		}State;
		State			state;
		CriticalSection	stateCS;
		Event			*suspension_lock;	//	blocks cores upon suspend().
		Semaphore		*stop_sem;			//	blocks the rMem until all cores terminate.
		Semaphore		*suspend_sem;		//	blocks the rMem until all cores are suspended.

		P<Group>	_root;	//	holds everything.
		Code		*_stdin;
		Code		*_stdout;
		Code		*_self;

		void	reset();	//	clear the content of the mem.

		CriticalSection	object_registerCS;
		CriticalSection	objectsCS;

		//	Utilities.
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

		std::vector<Group	*>	initial_groups;	//	convenience; cleared after start();

		uint32	last_oid;
		uint32	probe_level;

		uint64	starting_time;

		virtual	void	init_timings(uint64	now)=0;

		_Mem();
	public:
		static	_Mem	*Get(){	return	(_Mem	*)Mem::Get();	}

		typedef	enum{
			STDIN=0,
			STDOUT=1
		}STDGroupID;

		virtual	~_Mem();

		void	init(uint32	base_period,	//	in us.
					uint32	reduction_core_count,
					uint32	time_core_count,
					uint32	probe_level,
					uint32	ntf_mk_res,	//	resilience for notifications markers.
					uint32	goal_res,	//	resilience for goals.
					uint32	asmp_res,	//	resilience for assumptions.
					uint32	sim_res,	//	resilience for simulations.
					float32	float_tolerance,
					uint32	time_tolerance,
					uint64	goal_record_resilience);

		uint64	get_base_period()	const{	return	base_period;	}
		uint64	get_probe_level()	const{	return	probe_level;	}
		uint64	get_starting_time()	const{	return	starting_time;	}
		uint32	get_ntf_mk_res()	const{	return	ntf_mk_res;	}
		uint32	get_goal_res()		const{	return	goal_res;	}
		uint32	get_asmp_res()		const{	return	asmp_res;	}
		uint32	get_sim_res()		const{	return	sim_res;	}
		float32	get_float_tolerance()	const{	return	float_tolerance;	}
		uint32	get_time_tolerance()	const{	return	time_tolerance;	}
		uint64	get_goal_record_resilience()	const{	return	goal_record_resilience;	}

		Code	*get_root()		const;
		Code	*get_stdin()	const;
		Code	*get_stdout()	const;
		Code	*get_self()		const;

		void	wait_for_delegate();//	called by delegates just before performing their task.
		void	delegate_done();	//	called by delegates just after completion of their task.
		DState	check_state();		//	called by delegates after waiting in case stop() or suspend() are called in the meantime.
		void	start_core();		//	called upon creation of a delegate.
		void	shutdown_core();	//	called upon completion of a delegate's task.
		bool	suspend_core();		//	called by cores upon receiving a suspend job.

		uint64	start();	//	return the starting time.
		void	stop();		//	after stop() the content is cleared and one has to call load() and start() again.
		void	suspend();
		void	resume();

		uint32	get_oid();

		//	Called by groups at update time.
		//	Called by PGMOverlays at reduction time.
		//	Called by AntiPGMOverlays at signaling time and reduction time.
		virtual	void	inject_notification(View	*view,bool	lock)=0;

		//	Internal core processing	////////////////////////////////////////////////////////////////

		_ReductionJob	*popReductionJob();
		void			pushReductionJob(_ReductionJob	*j);
		TimeJob			*popTimeJob();
		void			pushTimeJob(TimeJob	*j);

		//	Called at each update period.
		//	- set the final resilience value, if 0, delete.
		//	- set the final saliency.
		//	- set the final activation.
		//	- set the final visibility, cov.
		//	- propagate saliency changes.
		//	- inject next update job for the group.
		//	- inject new signaling jobs if act pgm with no input or act |pgm.
		//	- notify high and low values.
		void	update(Group	*group);

		//	Called upon successful reduction.
		virtual	void	inject_new_object(View	*view)=0;
				void	inject_existing_object(View	*view,Code	*object,Group	*host,bool	lock);

		//	Called as a result of a group update (sln change).
		//	Calls mod_sln on the object's view with morphed sln changes.
		void	propagate_sln(Code	*object,float32	change,float32	source_sln_thr);

		//	Called by groups.
		void	inject_copy(View	*view,Group	*destination,uint64	now);	//	for cov; NB: no cov for groups, r-groups, models, pgm or notifications.
		void	inject_reduction_jobs(View	*view,Group	*host);	//	builds reduction jobs from host's inputs and own overlay (assuming host is c-salient and the view is salient);
																//	builds reduction jobs from host's inputs and viewing groups' overlays (assuming host is c-salient and the view is salient).

		//	Interface for overlays and I/O devices	////////////////////////////////////////////////////////////////
		virtual	void	inject(View	*view)=0;
		virtual	Code	*check_existence(Code	*object)=0;	//	returns the existing object if any, or object otherwise: in the latter case, packing may occur.

		//	rMem to rMem.
		//	The view must contain the destination group (either stdin or stdout) as its grp member.
		//	To be redefined by object transport aware subcalsses.
		virtual	void	eject(View	*view,uint16	nodeID);

		//	From rMem to I/O device.
		//	To be redefined by object transport aware subcalsses.
		virtual	void	eject(Code	*command);

		virtual	r_code::Code	*build_object(Atom	head)=0;

		//	production of abstracted code (cloned).
		r_code::Code	*clone_object(r_code::Code	*object);	//	views are cloned; markers are not.
		r_code::Code	*abstract_object_clone(r_code::Code	*object);
		r_code::Code	*abstract_object_clone(r_code::Code	*object,BindingMap	*bm);
		void			abstract_object_member_clone(r_code::Code	*object,uint16	index,BindingMap	*bm);

		//	producion of abstracted code (patched).
		void	abstract_high_level_pattern(r_code::Code	*object,BindingMap	*bm);
		void	abstract_object(r_code::Code	*object,BindingMap	*bm);
		void	abstract_object_member(r_code::Code	*object,uint16	index,BindingMap	*bm);
	};

	//	O is the class of the objects held by the rMem (except groups and notifications):
	//		r_exec::LObject if non distributed, or
	//		RObject (see the integration project) when network-aware.
	//	Notification objects and groups are instances of r_exec::LObject (they are not network-aware).
	//	Objects are built at reduction time as r_exec:LObjects and packed into instances of O when O is network-aware.
	//	Shared resources:
	//		Mem::object_register: accessed by Mem::update, Mem::injectNow and Mem::deleteObject (see above).
	template<class	O>	class	Mem:
	public	_Mem{
	protected:
		std::list<Code	*>											objects;			//	to insert in an image (getImage()); in order of injection.
		UNORDERED_SET<O	*,typename	O::Hash,typename	O::Equal>	object_register;	//	to eliminate duplicates (content-wise); does not include groups.

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

		//	Functions called by internal processing of jobs (see internal processing section below).
		void	inject_new_object(View	*view);	//	also called by inject() (see below).
	public:
		Mem();
		virtual	~Mem();

		//	Called by r_comp::Image.
		r_code::Code	*build_object(r_code::SysObject	*source);

		//	Called by operators and overlays.
		r_code::Code	*build_object(Atom	head);

		bool	load(std::vector<r_code::Code	*>	*objects);	//	call before start; no mod/set/eje will be executed (only inj);
																//	ijt will be set at now=Time::Get() whatever the source code.
																//	return false on error.
		void	delete_object(Code	*object);	//	called by object destructors/Group::clear().

		//	External device I/O	////////////////////////////////////////////////////////////////
		r_comp::Image	*get_image();	//	create an image; fill with all objects; call only when stopped.

		//	Executive device functions	////////////////////////////////////////////////////////
		
		//	Called by the reduction core.
		void	inject(View	*view);
		Code	*check_existence(Code	*object);

		//	Called by the communication device (I/O).
		void	inject(O	*object,View	*view);

		//	Variant of injectNow optimized for notifications.
		void	inject_notification(View	*view,bool	lock);

		//	Called by time cores.	////////////////////////////////////////////////////////////////
		void	update(SaliencyPropagationJob	*j);
	};

	r_exec_dll r_exec::Mem<r_exec::LObject> *Run(const	char	*user_operator_library_path,
		uint64			(*time_base)(),
		const	char	*seed_path,
		const	char	*source_file_name);
}


#include	"mem.tpl.cpp"


#endif