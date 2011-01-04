//	test_program.cpp
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

#include	"test_program.h"

#include	"../r_exec/mem.h"


//	Sample c++ user-defined program.
class	TestController:
public	r_exec::Controller{
private:
	float32	arg1;
	bool	arg2;
public:
	TestController(r_code::View	*icpp_pgm_view):r_exec::Controller(icpp_pgm_view){

		//	Load arguments here: one float and one Boolean.
		uint16	arg_set_index=getObject()->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=getObject()->code(arg_set_index).getAtomCount();
		if(arg_count!=2){

			std::cerr<<"test_program error: expected 2 arguments, got "<<arg_count<<std::endl;
			return;
		}
		arg1=getObject()->code(arg_set_index+1).asFloat();
		arg2=getObject()->code(arg_set_index+2).asBoolean();
	}

	~TestController(){
	}

	void	take_input(r_exec::View	*input,r_exec::Controller	*origin=NULL){

		//	Inputs are all types of objects - salient or that have become salient depending on their view's sync member.
		//	Manual filtering may be needed instead of pattern-matching.

		//input->object->trace();
	}
};

////////////////////////////////////////////////////////////////////////////////

r_exec::Controller	*test_program(r_code::View	*view){

	return	new	TestController(view);
}