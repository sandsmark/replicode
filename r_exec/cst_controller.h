//	cst_controller.h
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

#ifndef	cst_controller_h
#define	cst_controller_h

#include	"hlp_overlay.h"
#include	"hlp_controller.h"


namespace	r_exec{

	class	CSTOverlay:
	public	HLPOverlay{
	protected:
		uint64	birth_time;

		std::vector<P<Code> >	inputs;

		void	inject_production();

		CSTOverlay(const	CSTOverlay	*original);
	public:
		CSTOverlay(Controller	*c,BindingMap	*bindings,uint8	reduction_mode);
		~CSTOverlay();

		Overlay	*reduce(View	*input);

		void	load_patterns();

		uint64	get_birth_time()	const{	return	birth_time;	}
	};

	class	CSTController:
	public	HLPController{
	private:
		void	produce_goals(Code	*super_goal,BindingMap	*bm);
	public:
		CSTController(r_code::View	*view);
		~CSTController();

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);
		void	produce_goals(Code	*super_goal,BindingMap	*bm,Code	*excluded_pattern);

		uint16	get_instance_opcode()	const;
	};
}


#endif