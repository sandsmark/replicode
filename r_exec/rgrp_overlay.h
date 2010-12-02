//	rgrp_overlay.h
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

#ifndef	r_group_overlay_h
#define	r_group_overlay_h

#include	"overlay.h"
#include	"r_group.h"
#include	"model.h"


namespace	r_exec{

	class	FwdController;
	class	RGRPMasterOverlay;

	//	Overlays are used to index the value held by variable objects (bindings, stored per overlay) - bindings include nil values (i.e. yet unbound variables).
	//	Like program overlays hold a list of patterns that have stil to be matched, r-group overlays hold a list of the binders that still have to match.
	//	Binders are assumed to be the only ipgms held by r-groups.
	class	r_exec_dll	RGRPOverlay:
	public	Overlay{
	friend	class	RGRPMasterOverlay;
	private:
		uint64	birth_time;	//	same purpose as for pgm overlays.

		std::list<P<View> >				binders;	//	binders which remain to execute (a binder can execute only once per overlay).
		UNORDERED_MAP<Code	*,P<Code> >	bindings;	//	variable|value.
		std::vector<Code	*>			last_bound;	//	variables.
		uint16							unbound_var_count;
		bool							discard_bindings;
		uint8							reduction_mode;	//	initialized by a master overlay.

		RGRPOverlay(__Controller	*c,RGroup	*group,UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode);
		RGRPOverlay(RGRPOverlay	*original);
	public:
		~RGRPOverlay();

		void	reduce(r_exec::View	*input);
		void	bind(Code	*value,Code	*var);
	};

	//	Master r-group overlays.
	//	Acts as a sub-controller.
	//	See FwdController comments.
	class	r_exec_dll	RGRPMasterOverlay:
	public	_Controller<Overlay>{
	friend	class	FwdController;
	private:
		UNORDERED_MAP<Code	*,P<Code> >	bindings;	//	variable|value.

		RGRPMasterOverlay(FwdController	*c,Code	*mdl,RGroup	*rgrp,UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode);
	public:
		~RGRPMasterOverlay();

		void	reduce(r_exec::View	*input);
		void	fire(UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode);
	};

	class	Monitor:
	public	_Object{
	protected:
		Monitor();
	public:
		virtual	bool	is_alive()=0;
		virtual	bool	take_input(r_exec::View	*input)=0;	//	returns true in case of success.
		virtual	void	update()=0;	//	called by monitoring jobs.
	};

	//	Monitors one prediction.
	class	PMonitor:
	public	Monitor{
	private:
		FwdController	*controller;
		uint64	expected_time_high;
		uint64	expected_time_low;
		bool	success;	//	set to true when a positive match for the target was found in due time.
	public:
		P<Code>	target;	//	mk.pred.

		PMonitor(FwdController	*c,Code	*target,uint64	expected_time,uint64	time_tolerance);

		bool	is_alive();
		bool	take_input(r_exec::View	*input);
		void	update();
	};

	class	GSMonitor;

	//	Monitors one goal.
	class	GMonitor:
	public	Monitor{
	private:
		GSMonitor	*parent;
	public:
		P<Code>	target;	//	mk.goal.

		GMonitor(GSMonitor	*c,Code	*target);

		bool	is_alive();
		bool	take_input(r_exec::View	*input);
		void	update();
	};

	//	Monitors a set of goals.
	class	GSMonitor:
	public	Monitor{
	private:
		Model			*inv_model;
		FwdController	*controller;
		GSMonitor		*parent;

		bool	sim;
		bool	asmp;

		class	MonitorHash{
		public:
			size_t	operator	()(P<Monitor>	*p)	const{
				
				return	(size_t)(Monitor	*)*p;
			}
		};

		UNORDERED_MAP<P<Monitor>,uint64,typename	MonitorHash>	monitors;
		CriticalSection												monitorsCS;

		void	add_outcome(GMonitor	*m,bool	outcome);

		bool			alive;
		CriticalSection	aliveCS;
	public:
		GSMonitor(Model	*inv_model,FwdController	*c,GSMonitor	*parent,bool	sim,bool	asmp);

		void	kill();
		bool	is_alive();
		bool	take_input(r_exec::View	*input);
		void	update();

		void	add_monitor(GMonitor	*m);

		void	update(GMonitor	*m);
		void	register_outcome(GMonitor	*m,Code	*object);

		Code	*get_mk_sim(Code	*object)	const;
		Code	*get_mk_asmp(Code	*object)	const;
	};

	//	R-groups behave like programs: they take inputs from visible/(newly) salient views (depending on their sync mode).
	//	Overlays are built unpon the matching of at least one binder.
	//	Building an overlay means adding bindings (binding: a value for a given overlay) to variable objects.
	//	Binding is performed by the _subst function of the executive (see InputLessPGMOverlay::injectProductions()).
	//	The overlays held by controllers are r-group master overlays; these in turn hold r-group overlays.
	//	If the r-group has a parent, one master is built each time some parent overlay fires; otherwise there is only one master.
	//	If the parent overlay that built a master is killed (being too old), said master is removed from the controllers, along with all its overlays;
	//	The views of forward models hold a forward controller that takes inputs from the group the view dwells in.
	//	Predictions are produced with an infinite resilience. They are killed by PMonitors at the time their object is predicted to happen.
	class	r_exec_dll	FwdController:
	public	Controller{
	public:
		typedef	enum{
			HEAD=0,
			MIDDLE=1,
			TAIL=2
		}Position;
	private:
		RGroup	*rgrp;
		std::vector<P<View> >	pending_inputs;	//	stored before the controller becomes activated (i.e. before its parent fires).

		void	reduce(r_exec::View	*input);	//	convenience.
		void	inject_productions(UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode);

		std::list<P<Monitor> >	monitors;
		CriticalSection			monitorsCS;
	public:
		FwdController(r_code::View	*view);	//	view is either a r-grp view (grp_view) or a mdl view (pgm_view).
		~FwdController();

		RGroup		*get_rgrp()	const;
		Position	get_position()	const;

		void	take_input(r_exec::View	*input,Controller	*origin=NULL);
		void	activate(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,
						 UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings,
						 uint8							reduction_mode);
		void	fire(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,
					 UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings,
					 uint8							reduction_mode);
		
		//	Called back by monitors.
		void	add_outcome(PMonitor	*m,bool	success);

		void	add_monitor(Monitor	*m);
		void	remove_monitor(Monitor	*m);
	};

	//	The views of inverse models hold an inverse controller that takes inputs from the group the view dwells in.
	class	r_exec_dll	InvController:
	public	Controller{
	private:
		RGroup	*rgrp;
	public:
		InvController(r_code::View	*view);	//	view is a mdl view (pgm_view).
		~InvController();

		void	take_input(r_exec::View	*input,Controller	*origin=NULL);
	};
}


#include	"rgrp_overlay.inline.cpp"


#endif