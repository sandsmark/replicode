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

	class	View;
	class	Controller;

	class	r_exec_dll	Overlay:
	public	_Object{
	protected:
		bool			alive;
		Controller		*controller;

		Overlay();
		Overlay(Controller	*c);
	public:
		virtual	~Overlay();

		virtual	void	reset();							//	reset to original state.
		virtual	Overlay	*reduce(r_exec::View	*input);	//	returns an offspring in case of a match.

		void	kill();
		bool	is_alive();

		// Delegated to the controller.
		r_code::Code	*getObject()	const;
		r_exec::View	*getView()		const;
		r_code::Code	*build_object(Atom	head)	const;
	};

	//	Upon invocation of take_input() the overlays older than tsc are killed, assuming stc>0; otherwise, overlays live unitl the ipgm dies.
	//	Shared resources:
	//	- alive: written by TimeCores (via UpdateJob::update() and _Mem::update()), read by TimeCores (via SignalingJob::update()).
	//	- overlays: modified by take_input, executed by TimeCores (via UpdateJob::update() and _Mem::update()) and ReductionCore::Run() (via ReductionJob::update(), PGMOverlay::reduce(), _Mem::inject() and add()/remove()/restart()).
	//	Controllers are built at loading time and at the view's injection time.
	//	Derived classes must expose a function: void	reduce(r_code::View*input);	(called by reduction jobs).
	class	r_exec_dll	Controller:
	public	_Object{
	protected:
		bool			alive;
		CriticalSection	aliveCS;

		uint64	tsc;

		r_code::View	*view;

		CriticalSection	reductionCS;

		Controller(r_code::View	*view);
	public:
		virtual	~Controller();

		uint64	get_tsc(){	return	tsc;	}

		void	kill();
		bool	is_alive();

		r_code::Code	*getObject()	const;	// return the reduction object (e.g. ipgm, r-grp, icpp_pgm). The object must have a tsc.
		r_exec::View	*getView()		const;	// return the reduction object's view.

		virtual	void	take_input(r_exec::View	*input){}	// called by the rMem at update time and at injection time.
		template<class	C>	void	_take_input(r_exec::View	*input){	//	utility: to be called by sub-classes.

			ReductionJob<C>	*j=new	ReductionJob<C>(new	View(input),(C	*)this);
			_Mem::Get()->pushReductionJob(j);
		}

		virtual	void	gain_activation()	const{}
		virtual	void	lose_activation()	const{}
	};

	class	r_exec_dll	OController:
	public	Controller{
	protected:
		std::list<P<Overlay> >	overlays;

		OController(r_code::View	*view);
	public:
		virtual	~OController();
	};
}


#include	"overlay.inline.cpp"


#endif