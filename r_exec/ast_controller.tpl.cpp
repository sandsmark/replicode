//	ast_controller.tpl.cpp
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

#include	"ast_controller.h"
#include	"mem.h"
#include	"factory.h"
#include	"auto_focus.h"


namespace	r_exec{

	template<class	U>	ASTController<U>::ASTController(const	AutoFocusController	*auto_focus,_Fact	*target):OController(NULL){
		
		this->target=target;
		tpx=new	CTPX(auto_focus,(_Fact	*)target);	// target is the premise, i.e. the tpx' target to be defeated.
		thz=Now()-Utils::GetTimeTolerance();
	}

	template<class	U>	ASTController<U>::~ASTController(){
	}

	template<class	U>	void	ASTController<U>::take_input(r_exec::View	*input){
		
		if(is_invalidated())
			return;
		if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
			input->object->code(0).asOpcode()!=Opcodes::AntiFact)	// discard everything but facts and |facts.
			return;
		if(input->get_ijt()<thz)
			return;
		Controller::__take_input<U>(input);
	}

	template<class	U>	void	ASTController<U>::reduce(View	*input){

		if(is_invalidated())
			return;

		_Fact	*input_object=input->object;
		if(input_object->is_invalidated())
			return;
//uint64	t0=Now();
		reductionCS.enter();

		if(input_object==target){

			tpx->store_input(input_object);
			reductionCS.leave();
			return;
		}
		
		Pred	*prediction=input_object->get_pred();
		if(prediction){
			//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" TPX"<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" got pred "<<prediction->get_target()->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<std::endl;
			switch(prediction->get_target()->is_timeless_evidence(target)){
			case	MATCH_SUCCESS_POSITIVE:
			case	MATCH_SUCCESS_NEGATIVE:	// a model predicted the next value of the target.
				//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" TPX"<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" killed\n";
				kill();
				break;
			case	MATCH_FAILURE:
				break;
			}

			reductionCS.leave();
			return;
		}
//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" TPX"<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" got "<<input->object->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<std::endl;
		((U	*)this)->reduce(input,input_object);

		reductionCS.leave();
//uint64	t1=Now();
//std::cout<<"ASTC: "<<t1-t0<<std::endl;
	}

	template<class	U>	void	ASTController<U>::kill(){

		invalidate();
		getView()->force_res(0);
	}
}