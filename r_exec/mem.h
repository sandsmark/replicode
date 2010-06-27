//	mem.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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


namespace	r_exec{

	class	r_exec_dll	_Mem:
	public	r_code::Mem{
	public:
		typedef	enum{
			NOT_STARTED=0,
			STARTED=1,
			STOPPED=2
		}State;
	protected:
		uint32	base_period;

		PipeNN<ReductionJob,1024>	*reduction_job_queue;
		PipeNN<TimeJob,1024>		*time_job_queue;
		
		uint32			reduction_core_count;
		ReductionCore	**reduction_cores;
		uint32			time_core_count;
		TimeCore		**time_cores;

		std::list<DelegatedCore	*>	delegates;

		State			state;
		FastSemaphore	*state_sem;

		P<Group>	root;	//	holds everything.

		void	reset();	//	clear the content of the mem.

		virtual	void	injectNow(View	*view)=0;
		virtual	void	injectCopyNow(View	*view,Group	*destination)=0;
		virtual	void	update(Group	*group)=0;

		FastSemaphore	*object_register_sem;
		FastSemaphore	*objects_sem;

		_Mem();
	public:
		typedef	enum{
			STDIN=0,
			STDOUT=1
		}STDGroupID;

		virtual	~_Mem();

		void	init(uint32	base_period,	//	in us; same for upr, spr and res.
					uint32	reduction_core_count,
					uint32	time_core_count);

		uint64	get_base_period()	const{	return	base_period;	}

		void	add_delegate(uint64	dealine,_TimeJob	*j);
		void	remove_delegate(DelegatedCore	*core);
		void	stop();	//	after stop() the content is cleared and one has to call load() and start() again.

		State	get_state();

		//	Called by groups at update time.
		virtual	void	injectNotificationNow(View	*view,bool	lock)=0;

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
		void	update(InjectionJob	*j);

		//	Called each time an object propagates saliency changes.
		virtual	void	update(SaliencyPropagationJob	*j)=0;

		//	Interface for overlays	and I/O devices	////////////////////////////////////////////////////////////////
		virtual	void	inject(View	*view)=0;

		//	rMem to rMem.
		//	The view must contain the destintion group (either stdin or stdout) as its grp member.
		//	To be redefined by object transport aware subcalsses.
		virtual	void	eject(View	*view,uint16	nodeID);

		//	From rMem to I/O device.
		//	To be redefined by object transport aware subcalsses.
		virtual	void	eject(Code	*command,uint16	nodeID);

		virtual	r_code::Code	*buildObject(Atom	head)=0;
	};

	//	O is the class of the objects held by the Mem.
	//	Shared resources:
	//		Mem::object_register: accessed by Mem::update, Mem::injectNow and Mem::deleteObject (see above).
	template<class	O>	class	Mem:
	public	_Mem{
	private:
		std::list<Code	*>											objects;			//	to insert in an image (getImage()); in order of injection.
		UNORDERED_SET<O	*,typename	O::Hash,typename	O::Equal>	object_register;	//	to eliminate duplicates (content-wise); does not include groups.

		//	Functions called by internal processing of jobs (see internal processing section below).
		void	injectNow(View	*view);													//	also called by inject() (see below).
		void	injectCopyNow(View	*view,Group	*destination);							//	for cov; NB: no cov for groups, pgm or notifications.
		void	update(Group	*group);												//	checks for exiting objects and injects.
		void	propagate_sln(O	*object,float32	change,float32	source_sln_thr);	//	calls mod_sln on the object's view with morphed sln changes.

		//	Utilities.
		void	_inject_group_now(View	*view,Group	*object,Group	*host);
		void	_inject_existing_object_now(View	*view,O	*object,Group	*host);
		void	_inject_reduction_jobs(View	*view,Group	*host);	//	builds reduction jobs from host's inputs and own overlay (assuming host is c-salient and the view is salient);
																//	builds reduction jobs from host's inputs and viewing groups' overlays (assuming host is c-salient and the view is salient).

		void	_initiate_sln_propagation(O	*object,float32	change,float32	source_sln_thr);
		void	_initiate_sln_propagation(O	*object,float32	change,float32	source_sln_thr,std::vector<O	*>	&path);
		void	_propagate_sln(O	*object,float32	change,float32	source_sln_thr,std::vector<O	*>	&path);

		std::vector<Group	*>	initial_groups;	//	convenience; cleared after start();
	protected:
		Group	*_stdin;	//	convenience.
		Group	*_stdout;	//	convenience.
		O		*_self;		//	convenience.
	public:
		Mem();
		virtual	~Mem();

		//	Called by r_comp::Image.
		r_code::Code	*buildObject(r_code::SysObject	*source);

		//	Called by operators and overlays.
		r_code::Code	*buildObject(Atom	head);

		void	load(std::vector<r_code::Code	*>	*objects);	//	call before start; 0: root, 1.stdin, 2:stdout, 3:self: these objects must be defined in the source code in that order; no mod/set/eje will be executed (only inj); ijt will be set at now=Time::Get() whatever the source code.
		void	start();

		void	deleteObject(Code	*object);

		Group	*get_stdin()	const;
		Group	*get_stdout()	const;

		//	External device I/O	////////////////////////////////////////////////////////////////
		r_comp::Image	*getImage();	//	create an image; fill with all objects; call only when suspended/stopped.

		//	Executive device functions	////////////////////////////////////////////////////////
		
		//	Called by the reduction core.
		void	inject(View	*view);

		//	Called by the communication device (I/O).
		void	inject(O	*object,View	*view);

		//	Variant of injectNow optimized for notifications.
		void	injectNotificationNow(View	*view,bool	lock);

		//	Called by cores.	////////////////////////////////////////////////////////////////
		void	update(SaliencyPropagationJob	*j);
	};
}


#include	"mem.tpl.cpp"


#endif