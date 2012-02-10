//	pattern_extractor.cpp
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

#include	"auto_focus.h"
#include	"reduction_job.h"
#include	"mem.h"


namespace	r_exec{

	TPX::TPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_Object(),controller(controller),target(Input(target,pattern,bindings)){
	}

	TPX::TPX(const	TPX	*original):_Object(),controller(original->controller),target(Input(original->target.input,original->target.abstraction,original->target.bindings)){
	}

	TPX::~TPX(){
	}

	bool	TPX::take_input(Input	*input){

		return	input->bindings->intersect(target.bindings);
	}

	void	TPX::signal(View	*input)	const{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_TPX::_TPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):TPX(controller,target,pattern,bindings){
	}

	_TPX::~_TPX(){
	}

	bool	_TPX::take_input(Input	*input){	// push new input in the time-controlled buffer; old inputs are in front.

		if(!input->bindings->intersect(target.bindings))
			return	false;

		uint64	now=Now();
		uint64	THZ=_Mem::Get()->get_tpx_time_horizon();

		std::list<Input>::iterator	i;
		for(i=inputs.begin();i!=inputs.end();){	// trim the buffer down.

			if((*i).input->is_invalidated())
				i=inputs.erase(i);
			else{

				uint64	after=Utils::GetTimestamp<Code>((*i).input,FACT_AFTER);
				if(now-after>THZ)
					i=inputs.erase(i);
				else	// after this point all inputs are young enough.
					break;
			}
		}
		inputs.push_back(*input);
	}


	void	_TPX::reduce(View	*input){

		Code	*mdl=build_model();
		if(mdl){

			_Mem::Get()->pack_hlp(mdl);
			controller->inject_hlp(mdl);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GTPX::GTPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_TPX(controller,target,pattern,bindings){
	}

	GTPX::~GTPX(){
	}

	void	GTPX::signal(View	*input)	const{	// will be erased from the AF map upon return. P<> kept in reduction job.

		if(((_Fact	*)input->object)->is_fact()){	// goal success.

			ReductionJob<GTPX>	*j=new	ReductionJob<GTPX>(new	View(input),(GTPX	*)this);
			_Mem::Get()->pushReductionJob(j);
		}
	}

	Code	*GTPX::build_model(){

		// TODO. success->p->f and f==target, means that a mdl can solve the goal: return NULL.
		// else: take the input before the goal success and build a model; in the process, identify new CST if LHS and inject.
		// if selected input==success->f->pred->x by mdl M, use f->imdl M as the lhs of the new model.
		return	NULL;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PTPX::PTPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_TPX(controller,target,pattern,bindings){
	}

	PTPX::~PTPX(){
	}

	void	PTPX::signal(View	*input)	const{	// will be erased from the AF map upon return. P<> kept in reduction job.

		if(input->object->code(0).asOpcode()==Opcodes::AntiFact){	// prediction failure.

			ReductionJob<PTPX>	*j=new	ReductionJob<PTPX>(new	View(input),(PTPX	*)this);
			_Mem::Get()->pushReductionJob(j);
		}
	}


	Code	*PTPX::build_model(){

		// TODO. success->p->f and f==target, means that a mdl can anticipate the failure of the pred: return NULL.
		// else: take the input before the pred failure and build a model (s_r); in the process, identify new CST if LHS and inject.
		// if selected input==success->f->pred->x by mdl M, use f->imdl M as the lhs of the new model.
		return	NULL;
	}
}