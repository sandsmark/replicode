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
#include	"group.h"


namespace	r_exec{

	//	Basically, an r-grp overlay just holds its birth time.
	//	Overlays are used to index the value held by variable objects (bindings are per overlay).
	class	r_exec_dll	RGRPOverlay:
	public	Overlay{
	private:
		uint64	birth_time;
	public:
		void	reduce(r_exec::View	*input);
	};

	//	R-groups behave like programs: they take inputs from visible/(newly) salient views depending on their sync mode.
	//	Overlays are built unpon the binding of at least one variable object.
	//	Building an overlay means adding bindings (binding: a value for a given overlay) to variable objects.
	//	Binding is performed by the _subst function of the executive (see InputLessPGMOverlay::injectProductions()).
	class	r_exec_dll	RGRPController:
	public	Controller{
	private:
		r_code::View	*rgrp_view;
	public:
		RGRPController(_Mem	*m,r_code::View	*rgrp_view);
		~RGRPController();

		std::vector<P<Group> >	ntf_groups;
		std::vector<P<Group> >	out_groups;

		void	take_input(r_exec::View	*input,Controller	*origin=NULL);
	};
}


#include	"rgrp_overlay.inline.cpp"


#endif