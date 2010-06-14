#ifndef	mem_h
#define	mem_h

#include	"reduction_core.h"
#include	"time_core.h"

#include	"../r_comp/segments.h"

#include	"pipe.h"


namespace	r_exec{

	//	To be implemented by a communication device.
	class	dll_export	Comm{
	public:
		typedef	enum{
			STDIN=0,
			STDOUT=1
		}STDGroupID;
		virtual	void	eject(Object	*object,uint16	nodeID,STDGroupID	destination)=0;	//	rMem to rMem.
		virtual	void	eject(Object	*object,uint16	nodeID)=0;							//	rMem to I/O device.
	};

	//	Shared resources:
	//		Mem::object_io_map: accessed by Mem::retrieve (called by I/O threads), Mem::injectNow (i.e. reduction cores via overlays) and Mem::deleteObject (dctor of Object).
	//		Mem::object_register: accessed by Mem::update, Mem::injectNow and Mem::deleteObject (see above).
	class	dll_export	Mem:
	public	r_code::Mem{
	private:
		static	SharedLibrary	UserLibrary;	//	user-defined operators.
		static	uint64	(*_Now)();				//	time base.

		uint32	base_period;

		PipeNN<ReductionJob,1024>	reduction_job_queue;
		PipeNN<TimeJob,1024>		time_job_queue;
		
		uint32			reduction_core_count;
		ReductionCore	**reduction_cores;
		uint32			time_core_count;
		TimeCore		**time_cores;

		P<Group>	root;		//	holds everything.
		Group		*_stdin;	//	convenience.
		Group		*_stdout;	//	convenience.
		Object		*_self;		//	convenience.

		UNORDERED_MAP<uint32,Object	*>							object_io_map;		//	[OID|object	*]: used for retrieving objects by ID; does not include groups; used only if comm!=NULL.
		UNORDERED_SET<Object	*,Object::Hash,Object::Equal>	object_register;	//	to eliminate duplicates (content-wise); does not include groups.

		Comm	*comm;	//	performs actual ejections.

		//	Functions called by internal processing of jobs (see internal processing section below).
		void	injectNow(View	*view);														//	also called by inject() (see below).
		void	injectCopyNow(View	*view,Group	*destination);								//	for cov; NB: no cov for groups, pgm or notifications.
		void	update(Group	*group);													//	checks for exiting objects and injects.
		void	propagate_sln(Object	*object,float32	change,float32	source_sln_thr);	//	calls mod_sln on the object's view with morphed sln changes.

		//	Utilities.
		void	_inject_group_now(View	*view,Group	*object,Group	*host);
		void	_inject_existing_object_now(View	*view,Object	*object,Group	*host);
		void	_inject_reduction_jobs(View	*view,Group	*host);	//	builds reduction jobs from host's inputs and own overlay (assuming host is c-salient and the view is salient);
																//	builds reduction jobs from host's inputs and viewing groups' overlays (assuming host is c-salient and the view is salient).
		void	_initiate_sln_propagation(Object	*object,float32	change,float32	source_sln_thr);
		void	_initiate_sln_propagation(Object	*object,float32	change,float32	source_sln_thr,std::vector<Object	*>	&path);
		void	_propagate_sln(Object	*object,float32	change,float32	source_sln_thr,std::vector<Object	*>	&path);

		std::vector<Group	*>	initial_groups;	//	convenience; cleared after start();

		FastSemaphore	*object_register_sem;
		FastSemaphore	*object_io_map_sem;
	public:
		static	void	Init(r_comp::ClassImage	*class_image,uint64	(*time_base)(),const	char	*user_operator_library_path);	//	load opcodes and std operators.
		static	uint64	Now(){	return	_Now();	}

		Mem(uint32	base_period,	//	in us; same for upr, spr and res.
			uint32	reduction_core_count,
			uint32	time_core_count,
			Comm	*comm);
		~Mem();

		uint64	get_base_period()	const{	return	base_period;	}

		//	r_code interface implementation.
		r_code::Object	*buildObject(r_code::SysObject	*source);
		r_code::Object	*buildGroup(r_code::SysObject	*source);
		r_code::Object	*buildInstantiatedProgram(r_code::SysObject	*source);
		r_code::Object	*buildMarker(r_code::SysObject	*source);

		void	init(std::vector<r_code::Object	*>	*objects);	//	call before start
																//	0: root, 1.stdin, 2:stdout, 3:self: these objects must be defined in the source code in that order.
																//	no mod/set/eje will be executed (only inj). ijt will be set at now=Time::Get() whatever the source code.
		void	start();
		void	stop();
		void	suspend();
		void	resume();

		ReductionJob	popReductionJob();
		void			pushReductionJob(ReductionJob	j);
		TimeJob			popTimeJob();
		void			pushTimeJob(TimeJob	j);

		void	deleteObject(Object	*object);

		//	External device I/O	////////////////////////////////////////////////////////////////
		Object	*retrieve(uint32	OID);
		r_comp::CodeImage	*getCodeImage();	//	create an image; fill with all objects; call only when suspended/stopped.

		//	Executive device functions	////////////////////////////////////////////////////////
		
		//	Called by the reduction core.
		void	inject(View	*view);

		//	Called by the communication device (I/O).
		void	inject(Object	*object,View	*view);

		//	Called by groups at update time.
		void	injectNotificationNow(View	*view);	//	variant of injectNow optimized for notifications.

		//	Called by the reduction core.
		//	Delegated to the communiction device (I/O).
		//	The view must contain the destintion group (either stdin or stdout) as its grp member.
		void	eject(View	*view,uint16	nodeID);

		//	Called by the reduction core.
		//	Delegated to the communiction device (I/O).
		void	eject(Object	*command,uint16	nodeID);

		//	Internal processing	////////////////////////////////////////////////////////////////

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
		void	update(SaliencyPropagationJob	*j);
	};
}


#include	"object.inline.cpp"


#endif