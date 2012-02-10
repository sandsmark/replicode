//	hlp_overlay.h
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

#ifndef	hlp_overlay_h
#define	hlp_overlay_h

#include	"overlay.h"
#include	"binding_map.h"


namespace	r_exec{

	class	HLPContext;

	//	HLP: high-level patterns.
	class	HLPOverlay:
	public	Overlay{
	friend	class	HLPContext;
	protected:
		P<BindingMap>	bindings;

		std::list<P<_Fact>	>	patterns;

		bool	evaluate_guards(uint16	guard_set_iptr_index);
		bool	evaluate_fwd_guards();
		bool	evaluate(uint16	index);
	public:
		HLPOverlay(Controller	*c,const	BindingMap	*bindings,bool	load_code=false);
		virtual	~HLPOverlay();

		BindingMap	*get_bindings()	const{	return	bindings;	}

		Atom	*get_value_code(uint16	id)	const;
		uint16	get_value_code_size(uint16	id)	const;

		Code	*get_unpacked_object()	const;

		bool	evaluate_bwd_guards();
	};
}


#endif