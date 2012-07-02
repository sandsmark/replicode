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
	
	// Upon invocation of take_input() the overlays older than tsc are killed, assuming stc>0; otherwise, overlays live unitl the ipgm dies.
	// Controllers are built at loading time and at the view's injection time.
	// Derived classes must expose a function: void	reduce(r_code::View*input);	(called by reduction jobs).
	class	r_exec_dll	Controller:
	public	_Object{
	protected:
		volatile	uint32	invalidated;	// 32 bit alignment.
		volatile	uint32	activated;		// 32 bit alignment.

		uint64	tsc;

		r_code::View	*view;

		CriticalSection	reductionCS;

		virtual	void	take_input(r_exec::View	*input){}
		template<class	C>	void	__take_input(r_exec::View	*input){	// utility: to be called by sub-classes.

			View	*_view=new	View(input);
			ReductionJob<C>	*j=new	ReductionJob<C>(_view,(C	*)this);
			_Mem::Get()->pushReductionJob(j);
		}

		Controller(r_code::View	*view);
	public:
		virtual	~Controller();

		uint64	get_tsc(){	return	tsc;	}

		virtual	void	invalidate()	{	invalidated=1;	}
		bool	is_invalidated()		{	return	invalidated==1;	};
		void	activate(bool	a)		{	activated=(a?1:0);	}
		bool	is_activated()	const	{	return	activated==1;	}
		bool	is_alive()	const		{	return	invalidated==0	&&	activated==1;	}

		virtual	Code	*get_core_object()	const=0;

		r_code::Code	*getObject()	const	{	return	view->object;			}	// return the reduction object (e.g. ipgm, icpp_pgm, cst, mdl).
		r_exec::View	*getView()		const	{	return	(r_exec::View	*)view;	}	// return the reduction object's view.

		void	_take_input(r_exec::View	*input);	// called by the rMem at update time and at injection time.

		virtual	void	gain_activation(){	activate(true);	 }
		virtual	void	lose_activation(){	activate(false); }

		void	set_view(View	*view);
	};

	class	_Context;
	class	IPGMContext;
	class	HLPContext;

	class	r_exec_dll	Overlay:
	public	_Object{
	friend	class	_Context;
	friend	class	IPGMContext;
	friend	class	HLPContext;
	protected:
		volatile	uint32	invalidated;

		Controller	*controller;

		r_code::vector<Atom>	values;	// value array: stores the results of computations.
		// Copy of the pgm/hlp code. Will be patched during matching and evaluation:
		// any area indexed by a vl_ptr will be overwritten with:
		// 		the evaluation result if it fits in a single atom,
		//		a ptr to the value array if the result is larger than a single atom,
		//		a ptr to an input if the result is a pattern input.
		Atom				*code;
		uint16				code_size;
		std::vector<uint16>	patch_indices;		// indices where patches are applied; used for rollbacks.
		uint16				value_commit_index;	// index of the last computed value+1; used for rollbacks.

		void				load_code();
		void				patch_code(uint16	index,Atom	value);
		uint16				get_last_patch_index();
		void				unpatch_code(uint16	patch_index);

		void	rollback();	// reset the overlay to the last commited state: unpatch code and values.
		void	commit();	// empty the patch_indices and set value_commit_index to values.size().

		Code	*get_core_object()	const;	// pgm, mdl, cst.

		Overlay();
		Overlay(Controller	*c,bool	load_code=true);
	public:
		virtual	~Overlay();

		virtual	void	reset();							// reset to original state.
		virtual	Overlay	*reduce(r_exec::View	*input);	// returns an offspring in case of a match.

				void	invalidate()		{	invalidated=1;	}
		virtual	bool	is_invalidated()	{	return	invalidated==1;	}

		r_code::Code	*getObject()	const	{	return	((Controller	*)controller)->getObject();	}
		r_exec::View	*getView()		const	{	return	((Controller	*)controller)->getView();	}

		r_code::Code	*build_object(Atom	head)	const;
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


#endif