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
#include	"dll.h"

#include	<list>

#include	"../r_comp/segments.h"

#include	"../../CoreLibrary/trunk/CoreLibrary/pipe.h"

#include	"pgm_overlay.h"


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

		PipeNN<P<_ReductionJob>,1024>	*reduction_job_queue;
		PipeNN<P<TimeJob>,1024>			*time_job_queue;
		
		uint32			reduction_core_count;
		ReductionCore	**reduction_cores;
		uint32			time_core_count;
		TimeCore		**time_cores;
uint32	CoreCount;
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

		P<Group>	root;	//	holds everything.

		void	reset();	//	clear the content of the mem.

		CriticalSection	object_registerCS;
		CriticalSection	objectsCS;

		void	injectCopyNow(View	*view,Group	*destination,uint64	now);	//	for cov; NB: no cov for groups, pgm or notifications.

		//	Utilities.
		void	_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr);
		void	_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path);
		void	_propagate_sln(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path);
		void	_inject_reduction_jobs(View	*view,Group	*host,Controller	*origin=NULL);	//	builds reduction jobs from host's inputs and own overlay (assuming host is c-salient and the view is salient);
																								//	builds reduction jobs from host's inputs and viewing groups' overlays (assuming host is c-salient and the view is salient).
		std::vector<Group	*>	initial_groups;	//	convenience; cleared after start();

		uint32	last_oid;

		_Mem();
	public:
		typedef	enum{
			STDIN=0,
			STDOUT=1
		}STDGroupID;

		virtual	~_Mem();

		void	init(uint32	base_period,	//	in us; same for upr, spr and res.
					uint32	reduction_core_count,
					uint32	time_core_count,
					uint32	ntf_mk_res=1);	//	resilience for notifications markers; for debugging purposes, use a long resilience.

		uint64	get_base_period()	const{	return	base_period;	}

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
		virtual	void	injectNotificationNow(View	*view,bool	lock,Controller	*origin=NULL)=0;

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
		virtual	void	injectNow(View	*view)=0;
		virtual	void	injectGroupNow(View	*view,Group	*object,Group	*host)=0;
		void	injectExistingObjectNow(View	*view,Code	*object,Group	*host,bool	lock);

		//	Called as a result of a group update (sln change).
		//	Calls mod_sln on the object's view with morphed sln changes.
		void	propagate_sln(Code	*object,float32	change,float32	source_sln_thr);

		//	Interface for overlays and I/O devices	////////////////////////////////////////////////////////////////
		virtual	void	inject(View	*view)=0;
		virtual	Code	*check_existence(Code	*object)=0;	//	returns the existing object if any, or object otherwise: in the latter case, packing may occur.

		//	rMem to rMem.
		//	The view must contain the destination group (either stdin or stdout) as its grp member.
		//	To be redefined by object transport aware subcalsses.
		virtual	void	eject(View	*view,uint16	nodeID);

		//	From rMem to I/O device.
		//	To be redefined by object transport aware subcalsses.
		virtual	void	eject(Code	*command,uint16	nodeID);

		virtual	r_code::Code	*buildObject(Atom	head)=0;
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
	private:
		std::list<Code	*>											objects;			//	to insert in an image (getImage()); in order of injection.
		UNORDERED_SET<O	*,typename	O::Hash,typename	O::Equal>	object_register;	//	to eliminate duplicates (content-wise); does not include groups.

		//	Functions called by internal processing of jobs (see internal processing section below).
		void	injectNow(View	*view);	//	also called by inject() (see below).
		void	injectGroupNow(View	*view,Group	*object,Group	*host);
	protected:
		Group	*_stdin;	//	convenience.
		Group	*_stdout;	//	convenience.
		O		*_self;		//	convenience (unused for now).
	public:
		Mem();
		virtual	~Mem();

		//	Called by r_comp::Image.
		r_code::Code	*buildObject(r_code::SysObject	*source);

		//	Called by operators and overlays.
		r_code::Code	*buildObject(Atom	head);

		bool	load(std::vector<r_code::Code	*>	*objects);	//	call before start; no mod/set/eje will be executed (only inj);
																//	ijt will be set at now=Time::Get() whatever the source code.
																//	return false on error.
		void	deleteObject(Code	*object);	//	called by object destructors/Group::clear().

		//	called upon reception of a remote object (for converting STDGroupID into actual objects).
		Group	*get_stdin()	const;
		Group	*get_stdout()	const;

		//	External device I/O	////////////////////////////////////////////////////////////////
		r_comp::Image	*getImage();	//	create an image; fill with all objects; call only when stopped.

		//	Executive device functions	////////////////////////////////////////////////////////
		
		//	Called by the reduction core.
		void	inject(View	*view);
		Code	*check_existence(Code	*object);

		//	Called by the communication device (I/O).
		void	inject(O	*object,View	*view);

		//	Variant of injectNow optimized for notifications.
		void	injectNotificationNow(View	*view,bool	lock,Controller	*origin=NULL);

		//	Called by time cores.	////////////////////////////////////////////////////////////////
		void	update(SaliencyPropagationJob	*j);
	};
}


#include	"mem.tpl.cpp"


#endif