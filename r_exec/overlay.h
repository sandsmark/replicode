//	overlay.h
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

#ifndef	overlay_h
#define	overlay_h

#include	"../../CoreLibrary/trunk/CoreLibrary/base.h"
#include	"../../CoreLibrary/trunk/CoreLibrary/utils.h"
#include	"../r_code/object.h"
#include	"dll.h"


using	namespace	r_code;

namespace	r_exec{

	class	_Mem;
	class	View;

	class	Controller;

	//	Unified overlay: derived in program overlay and rgroup overlay.
	//	Shared resources:
	//	- alive: read by ReductionCore (via reductionJob::update()), written by TimeCore (via the controller).
	class	r_exec_dll	Overlay:
	public	_Object{
	protected:
		bool			alive;
		CriticalSection	reductionCS;

		Controller	*controller;

		Overlay();
		Overlay(Controller	*c);
	public:
		virtual	~Overlay();

		void	kill();

		virtual	void	reset();	//	reset to original state.

		// Delegated to the controller.
		_Mem			*get_mem()		const;
		r_code::Code	*getObject()	const;
		r_exec::View	*getView()		const;

		r_code::Code	*buildObject(Atom	head)	const;

		virtual	void	reduce(r_exec::View	*input);
		virtual	bool	inject_productions(Controller	*origin)=0;	//	return true upon successful evaluation.
	};

	//	Unified controller: derived in program controller and rgroup controller.
	//	Upon invocation of take_input() the overlays older than tsc are killed, assuming stc>0; otherwise, overlays live unitl the ipgm dies.
	//	Shared resources:
	//	- alive: written by TimeCores (via UpdateJob::update() and _Mem::update()), read by TimeCores (via SignalingJob::update()).
	//	- overlays: modified by take_input, executed by TimeCores (via UpdateJob::update() and _Mem::update()) and ReductionCore::Run() (via ReductionJob::update(), PGMOverlay::reduce(), _Mem::inject() and add()/remove()/restart()).
	class	r_exec_dll	Controller:
	public	_Object{
	protected:
		r_code::View			*view;

		bool					alive;
		CriticalSection			aliveCS;

		std::list<P<Overlay> >	overlays;
		CriticalSection			overlayCS;

		uint64					tsc;

		_Mem					*mem;

		Controller(_Mem	*m,r_code::View	*view);
	public:
		virtual	~Controller();

		virtual	void	take_input(r_exec::View	*input,Controller	*origin=NULL);	// push one job for each overlay; called by the rMem at update time and at injection time.

		_Mem	*get_mem()	const;

		r_code::Code	*getObject()	const;	// return the reduction object (e.g. ipgm, r-grp, icpp_pgm). The object must have a tsc.
		r_exec::View	*getView()		const;	// return the reduction object's view.

		void	kill();
		bool	is_alive();

		void	add(Overlay	*overlay);
		void	remove(Overlay	*overlay);
	};
}


#include	"overlay.inline.cpp"


#endif