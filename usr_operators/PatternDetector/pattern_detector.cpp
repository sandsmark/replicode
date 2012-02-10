//	pattern_detector.cpp
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
/*DEPRECATED.
#include	"pattern_detector.h"

#include	"../r_exec/mem.h"
#include	"../r_exec/pattern_detector.h"


//	Proxy to its sibling in r_exec. Reason: avoid crossing dll boundaries (STL are involved).
class	PatternDetectorController:
public	r_exec::Controller{
private:
	P<r_exec::PatternDetector>	detector;
public:
	PatternDetectorController(r_code::View	*icpp_pgm_view):r_exec::Controller(icpp_pgm_view){

		//	Load arguments.
		uint16	arg_set_index=getObject()->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=getObject()->code(arg_set_index).getAtomCount();
		if(arg_count!=3){

			std::cerr<<"pattern_detector error: expected 2 arguments, got "<<arg_count<<std::endl;
			return;
		}
		r_exec::Group	*storage_group;
		if(getObject()->code(arg_set_index+1).readsAsNil())
			storage_group=NULL;
		else
			storage_group=(r_exec::Group	*)getObject()->get_reference(getObject()->code(arg_set_index+1).asIndex());
		uint16	output_groups_count=getObject()->code(arg_set_index+3).asFloat();
		uint16	output_group_set_index=getObject()->code(arg_set_index+2).asIndex();
		r_exec::Group	**tmp=new	r_exec::Group	*[output_groups_count];
		for(uint16	i=0;i<output_groups_count;++i)
			tmp[i]=(r_exec::Group	*)getObject()->get_reference(getObject()->code(output_group_set_index+i+1).asIndex());

		detector=r_exec::PatternDetector::New(storage_group,tmp,output_groups_count);
	}

	~PatternDetectorController(){
	}

	Code	*get_core_object()	const{	return	getObject()->get_reference(0);	}

	void	take_input(r_exec::View	*input){

		detector->take_input(input->object);
	}
};

////////////////////////////////////////////////////////////////////////////////

r_exec::Controller	*pattern_detector(r_code::View	*view){

	return	new	PatternDetectorController(view);
}*/