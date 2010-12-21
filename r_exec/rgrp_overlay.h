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


#define	RDX_MODE_REGULAR	0
#define	RDX_MODE_SIMULATION	1
#define	RDX_MODE_ASSUMPTION	2

namespace	r_exec{

	//	Unifies FwdOverlay, GSMonitor and _InvOverlay.
	class	r_exec_dll	RGRPOverlay:
	public	Overlay{
	protected:
		BindingMap	bindings;

		RGRPOverlay(__Controller	*c):Overlay(c){}
	public:
		virtual	void	bind(Code	*value_source,uint16	value_index,Code	*variable_source,uint16	variable_index)=0;
	};

	//	Unifies FwdOverlay and GSMonitor.
	//	Binding overlays are used to index the value held by variable objects (bindings, stored per overlay) - bindings include nil values (i.e. yet unbound variables).
	//	Like program overlays hold a list of patterns that have still to be matched, binding overlays hold a list of the binders that still have to match.
	//	Binders are assumed to be the only ipgms held by r-groups.
	class	r_exec_dll	BindingOverlay:
	public	RGRPOverlay{
	protected:
		uint64	birth_time;	//	same purpose as for pgm overlays.

		std::list<P<View> >		binders;	//	binders which remain to execute (a binder can execute only once per overlay).
		
		std::vector<Code	*>	last_bound_variable_objects;
		std::vector<Atom>		last_bound_code_variables;
		bool					discard_bindings;
		uint8					reduction_mode;	//	initialized by a master overlay.

		BindingOverlay(__Controller	*c,RGroup	*group,BindingMap	*bindings,uint8	reduction_mode);
		BindingOverlay(BindingOverlay	*original);
	public:
		virtual	~BindingOverlay();

		void	bind(Code	*value_source,uint16	value_index,Code	*variable_source,uint16	variable_index);
	};

	class	FwdController;
	class	RGRPMasterOverlay;

	class	r_exec_dll	FwdOverlay:
	public	BindingOverlay{
	friend	class	RGRPMasterOverlay;
	private:
		FwdOverlay(__Controller	*c,RGroup	*group,BindingMap	*bindings,uint8	reduction_mode);
		FwdOverlay(FwdOverlay	*original);
	public:
		~FwdOverlay();

		void	reduce(r_exec::View	*input);
	};

	//	Master r-group overlays.
	//	Acts as a sub-controller.
	//	See FwdController comments.
	class	r_exec_dll	RGRPMasterOverlay:
	public	_Controller<Overlay>{
	friend	class	FwdController;
	private:
		BindingMap	bindings;

		RGRPMasterOverlay(FwdController	*c,Code	*mdl,RGroup	*rgrp,BindingMap	*bindings,uint8	reduction_mode);
	public:
		~RGRPMasterOverlay();

		void	reduce(r_exec::View	*input);
		void	fire(BindingMap	*bindings,uint8	reduction_mode);
	};
}


#endif