//	overlay.cpp
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

#include	"overlay.h"
#include	"mem.h"


#define	MAX_VALUE_SIZE	128

namespace	r_exec{

	Overlay::Overlay():_Object(),invalidated(0){

		values.as_std()->resize(MAX_VALUE_SIZE);	// MAX_VALUE_SIZE is the limit; if the array is resized later on, some contexts with data==VALUE_ARRAY may point to invalid adresses: case of embedded contexts with both data==VALUE_ARRAY.
	}

	Overlay::Overlay(Controller	*c,bool	load_code):_Object(),controller(c),value_commit_index(0),code(NULL),invalidated(0){

		values.as_std()->resize(128);
		if(load_code)
			this->load_code();
	}

	Overlay::~Overlay(){

		if(code)
			delete[]	code;
	}

	inline	Code	*Overlay::get_core_object()	const{
		
		return	controller->get_core_object();
	}

	void	Overlay::load_code(){

		if(code)
			delete[]	code;

		Code	*object=get_core_object();
		//	copy the original pgm/hlp code.
		code_size=object->code_size();
		code=new	r_code::Atom[code_size];
		memcpy(code,&object->code(0),code_size*sizeof(r_code::Atom));
	}

	void	Overlay::reset(){

		memcpy(code,&getObject()->get_reference(0)->code(0),code_size*sizeof(r_code::Atom));	//	restore code to prisitne copy.
	}

	void	Overlay::rollback(){

		Code	*object=get_core_object();
		Atom	*original_code=&object->code(0);
		for(uint16	i=0;i<patch_indices.size();++i)	//	upatch code.
			code[patch_indices[i]]=original_code[patch_indices[i]];
		patch_indices.clear();

		if(value_commit_index!=values.size()){	//	shrink the values down to the last commit index.

			if(value_commit_index>0)
				values.as_std()->resize(value_commit_index);
			else
				values.as_std()->clear();
			value_commit_index=values.size();
		}
	}

	void	Overlay::commit(){

		patch_indices.clear();
		value_commit_index=values.size();
	}

	void	Overlay::patch_code(uint16	index,Atom	value){

		code[index]=value;
		patch_indices.push_back(index);
	}

	uint16	Overlay::get_last_patch_index(){

		return	patch_indices.size();
	}

	void	Overlay::unpatch_code(uint16	patch_index){

		Code	*object=get_core_object();
		Atom	*original_code=&object->code(0);
		for(uint16	i=patch_index;i<patch_indices.size();++i)
			code[patch_indices[i]]=original_code[patch_indices[i]];
		patch_indices.resize(patch_index);
	}

	Overlay	*Overlay::reduce(r_exec::View	*input){

		return	NULL;
	}

	r_code::Code	*Overlay::build_object(Atom	head)	const{
		
		return	_Mem::Get()->build_object(head);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Controller::Controller(r_code::View	*view):_Object(),invalidated(0),activated(0),view(view){

		if(!view)
			return;

		switch(getObject()->code(0).getDescriptor()){
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
		case	Atom::INSTANTIATED_ANTI_PROGRAM:
			tsc=Utils::GetTimestamp<Code>(getObject(),IPGM_TSC);
			break;
		case	Atom::INSTANTIATED_CPP_PROGRAM:
			tsc=Utils::GetTimestamp<Code>(getObject(),ICPP_PGM_TSC);
			break;
		}
	}

	Controller::~Controller(){
	}

	void	Controller::set_view(View	*view){

		this->view=view;
	}

	void	Controller::_take_input(r_exec::View	*input){	// called by the rMem at update time and at injection time.
	
		if(is_alive()	&&	!input->object->is_invalidated())
			take_input(input);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	OController::OController(r_code::View	*view):Controller(view){
	}

	OController::~OController(){
	}
}