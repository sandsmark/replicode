#ifndef	time_job_h
#define	time_job_h

#include	"object.h"
#include	"pgm_overlay.h"


namespace	r_exec{

	class	Mem;

	class	_TimeJob:
	public	r_code::_Object{
	public:
		virtual	void	update(Mem	*m)=0;
		virtual	bool	is_alive()	const;
	};
	
	class	TimeJob{
	public:
		int64				target_time;	//	0 means ASAP.
		r_code::P<_TimeJob>	job;
		TimeJob();
		TimeJob(_TimeJob	*j,uint64	ijt);
		bool	is_alive()	const;
	};

	class	UpdateJob:
	public	_TimeJob{
	public:
		r_code::P<Group>	group;
		UpdateJob(Group	*g);
		void	update(Mem	*m);
	};

	class	SignalingJob:
	public	_TimeJob{
	protected:
		SignalingJob(IPGMController	*o);
	public:
		r_code::P<IPGMController>	controller;
		bool	is_alive()	const;
	};

	class	AntiPGMSignalingJob:
	public	SignalingJob{
	public:
		AntiPGMSignalingJob(IPGMController	*o);
		void	update(Mem	*m);
	};

	class	InputLessPGMSignalingJob:
	public	SignalingJob{
	public:
		InputLessPGMSignalingJob(IPGMController	*o);
		void	update(Mem	*m);
	};

	class	InjectionJob:
	public	_TimeJob{
	public:
		r_code::P<View>	view;
		InjectionJob(View	*v);
		void	update(Mem	*m);
	};

	class	SaliencyPropagationJob:
	public	_TimeJob{
	public:
		r_code::P<Object>		object;
		float32					sln_change;
		float32					source_sln_thr;
		SaliencyPropagationJob(Object	*o,float32	sln_change,float32	source_sln_thr);
		void	update(Mem	*m);
	};
}


#endif