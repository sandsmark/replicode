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

#include	"pipe.h"

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
	protected:
		uint32	base_period;
		uint32	ntf_mk_res;

		PipeNN<ReductionJob,1024>	*reduction_job_queue;
		PipeNN<TimeJob,1024>		*time_job_queue;
		
		uint32			reduction_core_count;
		ReductionCore	**reduction_cores;
		uint32			time_core_count;
		TimeCore		**time_cores;
		uint32			delegate_count;

		typedef	enum{
			NOT_STARTED=0,
			RUNNING=1,
			SUSPENDED=2,
			STOPPED=3
		}State;
		State			state;
		FastSemaphore	*state_sem;
		Event			*suspension_lock;
		Semaphore		*stop_sem;

		P<Group>	root;	//	holds everything.

		void	reset();	//	clear the content of the mem.

		FastSemaphore	*object_register_sem;
		FastSemaphore	*objects_sem;

		virtual	void	injectNow(View	*view)=0;
		virtual	void	injectGroupNow(View	*view,Group	*object,Group	*host)=0;
		void	injectExistingObjectNow(View	*view,Code	*object,Group	*host,bool	lock);
		void	injectCopyNow(View	*view,Group	*destination,uint64	now);	//	for cov; NB: no cov for groups, pgm or notifications.
		void	update(Group	*group);									//	checks for exiting objects and injects.

		//	Utilities.
		void	propagate_sln(Code	*object,float32	change,float32	source_sln_thr);	//	calls mod_sln on the object's view with morphed sln changes.
		void	_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr);
		void	_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path);
		void	_propagate_sln(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path);
		void	_inject_reduction_jobs(View	*view,Group	*host,_PGMController	*origin=NULL);	//	builds reduction jobs from host's inputs and own overlay (assuming host is c-salient and the view is salient);
																								//	builds reduction jobs from host's inputs and viewing groups' overlays (assuming host is c-salient and the view is salient).
		std::vector<Group	*>	initial_groups;	//	convenience; cleared after start();

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

		void	add_delegate(uint64	dealine,_TimeJob	*j);	//	called by time cores when they are ahead of their deadlines.
		void	remove_delegate(DelegatedCore	*core);			//	called by delegate cores when they are done.
		bool	check_state(bool	is_delegate);

		void	start();
		void	stop();	//	after stop() the content is cleared and one has to call load() and start() again.
		void	suspend();
		void	resume();

		//	Called by groups at update time.
		//	Called by PGMOverlays at reduction time.
		//	Called by AntiPGMOverlays at signaling time and reduction time.
		virtual	void	injectNotificationNow(View	*view,bool	lock,_PGMController	*origin=NULL)=0;

		//	Internal core processing	////////////////////////////////////////////////////////////////

		ReductionJob	popReductionJob();
		void			pushReductionJob(ReductionJob	j);
		TimeJob			popTimeJob();
		void			pushTimeJob(TimeJob	j);

		//	Called at each update period.
		//	- set the final resilience value, if 0, delete.
		//	- set the final saliency.
		//	- set the final activation.
		//	- set the final visibility, cov.
		//	- propagate saliency changes.
		//	- inject next update job for the group.
		//	- inject new signaling jobs if act pgm with no input or act |pgm.
		//	- notify high and low values.
		void	update(UpdateJob	*j);

		//	Called each time an anti-ipgm reaches its time scope (tsc).
		void	update(AntiPGMSignalingJob	*j);

		//	Called at each signaling period for each active overlay with no inputs.
		void	update(InputLessPGMSignalingJob	*j);

		//	Called each time a view is to be injected in the future.
		void	update(InjectionJob		*j);	//	new object.
		void	update(EInjectionJob	*j);	//	existing object.
		void	update(GInjectionJob	*j);	//	group.

		//	Called each time an object propagates saliency changes.
		void	update(SaliencyPropagationJob	*j);

		//	Interface for overlays and I/O devices	////////////////////////////////////////////////////////////////
		virtual	Code	*inject(View	*view)=0;	//	returns the existing object if any.

		//	rMem to rMem.
		//	The view must contain the destintion group (either stdin or stdout) as its grp member.
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

		void	load(std::vector<r_code::Code	*>	*objects);	//	call before start; no mod/set/eje will be executed (only inj);
																//	ijt will be set at now=Time::Get() whatever the source code.
		void	deleteObject(Code	*object);	//	called by object destructors/Group::clear().

		//	called upon reception of a remote object (for converting STDGroupID into actual objects).
		Group	*get_stdin()	const;
		Group	*get_stdout()	const;

		//	External device I/O	////////////////////////////////////////////////////////////////
		r_comp::Image	*getImage();	//	create an image; fill with all objects; call only when stopped.

		//	Executive device functions	////////////////////////////////////////////////////////
		
		//	Called by the reduction core.
		Code	*inject(View	*view);

		//	Called by the communication device (I/O).
		void	inject(O	*object,View	*view);

		//	Variant of injectNow optimized for notifications.
		void	injectNotificationNow(View	*view,bool	lock,_PGMController	*origin=NULL);

		//	Called by time cores.	////////////////////////////////////////////////////////////////
		void	update(SaliencyPropagationJob	*j);
	};
}


#include	"mem.tpl.cpp"


#endif