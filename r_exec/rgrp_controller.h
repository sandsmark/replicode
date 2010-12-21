//	rgrp_controller.h
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

#ifndef	rgrp_controller_h
#define	rgrp_controller_h

#include	"rgrp_overlay.h"
#include	"monitor.h"


namespace	r_exec{

	//	R-groups behave like programs: they take inputs from visible/(newly) salient views (depending on their sync mode).
	//	Overlays are built unpon the matching of at least one binder.
	//	Building an overlay means adding bindings (binding: a value per given overlay) to variable objects.
	//	Binding is performed by the _subst function of the executive (see InputLessPGMOverlay::injectProductions()).
	//	The overlays held by controllers are r-group master overlays; these in turn hold r-group overlays.
	//	If the r-group has a parent, one master is built each time some parent overlay fires; otherwise there is only one master.
	//	If the parent overlay that built a master is killed (being too old), said master is removed from the controllers, along with all its overlays;
	//	The views of forward models hold a forward controller that takes inputs from the group the view dwells in.
	//	Predictions are produced with an infinite resilience. They are killed by PMonitors at the time their object is predicted to happen - whether it actually does or not.
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
		std::vector<P<View> >	pending_inputs;	//	stored before the controller becomes activated (i.e. before its parent fires) and flushed in upon activation.

		void	reduce(r_exec::View	*input);	//	convenience.
		void	inject_productions(BindingMap	*bindings,uint8	reduction_mode);

		std::list<P<PMonitor> >		p_monitors;
		CriticalSection				p_monitorsCS;

		std::list<P<GSMonitor> >	gs_monitors;
		CriticalSection				gs_monitorsCS;
	public:
		FwdController(r_code::View	*view);	//	view is either a r-grp view (grp_view) or a mdl view (pgm_view).
		~FwdController();

		RGroup		*get_rgrp()	const;
		Position	get_position()	const;

		void	take_input(r_exec::View	*input,Controller	*origin=NULL);
		void	activate(BindingMap	*overlay_bindings,BindingMap	*master_overlay_bindings,uint8		reduction_mode);
		void	fire(BindingMap	*overlay_bindings,BindingMap	*master_overlay_bindings,uint8		reduction_mode);
		
		//	Called back by monitors.
		void	add_outcome(PMonitor	*m,bool	success,float32	confidence);

		void	add_monitor(PMonitor		*m);
		void	remove_monitor(PMonitor		*m);
		void	add_monitor(GSMonitor		*m);
		void	remove_monitor(GSMonitor	*m);
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


#include	"rgrp_overlay.inline.cpp"	//	TODO: rename to rgrp_controller.inline.cpp.


#endif