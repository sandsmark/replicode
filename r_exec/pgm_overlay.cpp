//	pgm_overlay.cpp
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

#include	"pgm_overlay.h"
#include	"mem.h"
#include	"group.h"
#include	"init.h"
#include	"opcodes.h"
#include	"context.h"


//	pgm layout:
//
//	index											content
//
//	PGM_TPL_ARGS									>iptr to the tpl args set
//	PGM_INPUTS										>iptr to the inputs structured set
//	PGM_PRODS										>iptr to the production set
//	pgm_code[PGM_TPL_ARGS]							>tpl arg set #n0
//	pgm_code[PGM_TPL_ARGS]+1						>iptr to first tpl pattern
//	...												>...
//	pgm_code[PGM_TPL_ARGS]+n0						>iptr to last tpl pattern
//	pgm_code[pgm_code[PGM_TPL_ARGS]+1]				>opcode of the first tpl pattern
//	...												>...
//	pgm_code[PGM_INPUTS]							>inputs structured set
//	pgm_code[PGM_INPUTS]+1							>iptr to the input pattern set
//	pgm_code[PGM_INPUTS]+2							>iptr to the timing constraint set
//	pgm_code[PGM_INPUTS]+3							>iptr to the guard set
//	pgm_code[pgm_code[PGM_INPUTS]+1]				>input pattern set #n1
//	pgm_code[pgm_code[PGM_INPUTS]+1]+1				>iptr to first input pattern
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+1]+n1				>iptr to last input pattern
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+1]+1]	>opcode of the first input pattern
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+1]+n1]	>opcode of the last input pattern
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+2]				>timing constraint set #n2
//	pgm_code[pgm_code[PGM_INPUTS]+2]+1				>iptr to first timing constraint
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+2]+n2				>iptr to last timing constraint
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+2]+1]	>opcode of the first timing constraint
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+2]+n2]	>opcode of the last timing constraint
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+3]				>guard set #n3
//	pgm_code[pgm_code[PGM_INPUTS]+3]+1				>iptr to first guard
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+3]+n3				>iptr to last guard
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+3]+1]	>opcode of the first guard
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+3]+n3]	>opcode of the last guard
//	...												>...
//	pgm_code[pgm_code[PGM_PRODS]]					>production set #n4
//	pgm_code[pgm_code[PGM_PRODS]]+1					>iptr to first production
//	...												>...
//	pgm_code[pgm_code[PGM_PRODS]]+n4				>iptr to last production
//	pgm_code[pgm_code[pgm_code[PGM_PRODS]]+1]		>opcode of the first production
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_PRODS]]+n4]		>opcode of the last production
//	...												>...

namespace	r_exec{

	InputLessPGMOverlay::InputLessPGMOverlay():Overlay(){	//	used for constructing PGMOverlay offsprings.
	}

	InputLessPGMOverlay::InputLessPGMOverlay(Controller	*c):Overlay(c),value_commit_index(0){

		//	copy the original pgm code.
		pgm_code_size=getObject()->get_reference(0)->code_size();
		pgm_code=new	r_code::Atom[pgm_code_size];
		memcpy(pgm_code,&getObject()->get_reference(0)->code(0),pgm_code_size*sizeof(r_code::Atom));
		patch_tpl_args();
	}

	InputLessPGMOverlay::~InputLessPGMOverlay(){

		delete[]	pgm_code;
	}

	inline	void	InputLessPGMOverlay::reset(){

		memcpy(pgm_code,&getObject()->get_reference(0)->code(0),pgm_code_size*sizeof(r_code::Atom));	//	restore code to prisitne copy.
		patch_tpl_args();

		patch_indices.clear();
		value_commit_index=0;
		values.as_std()->clear();
		productions.clear();
	}

	inline	void	InputLessPGMOverlay::rollback(){

		Atom	*original_code=&getObject()->get_reference(0)->code(0);
		for(uint16	i=0;i<patch_indices.size();++i)	//	upatch code.
			pgm_code[patch_indices[i]]=original_code[patch_indices[i]];
		patch_indices.clear();

		if(value_commit_index!=values.size()){	//	shrink the values down to the last commit index.

			if(value_commit_index>0)
				values.as_std()->resize(value_commit_index);
			else
				values.as_std()->clear();
			value_commit_index=values.size();
		}
	}

	inline	void	InputLessPGMOverlay::commit(){

		patch_indices.clear();
		value_commit_index=values.size();
	}

	inline	bool	InputLessPGMOverlay::evaluate(uint16	index){

		Context	c(getObject()->get_reference(0),getView(),pgm_code,index,this);
		uint16	result_index;
		return	c.evaluate(result_index);
	}

	void	InputLessPGMOverlay::patch_tpl_args(){	//	no rollback on that part of the code.
//getObject()->trace();
		uint16	tpl_arg_set_index=pgm_code[PGM_TPL_ARGS].asIndex();			//	index to the set of all tpl patterns.
		uint16	arg_count=pgm_code[tpl_arg_set_index].getAtomCount();
		uint16	ipgm_arg_set_index=getObject()->code(IPGM_ARGS).asIndex();	//	index to the set of all ipgm tpl args.
		for(uint16	i=1;i<=arg_count;++i){									//	pgm_code[tpl_arg_set_index+i] is an iptr to a pattern.

			Atom	&skel_iptr=pgm_code[pgm_code[tpl_arg_set_index+i].asIndex()+1];
			patch_tpl_code(skel_iptr.asIndex(),getObject()->code(ipgm_arg_set_index+i).asIndex());
			skel_iptr=Atom::IPGMPointer(ipgm_arg_set_index+i);				//	patch the pgm code with ptrs to the tpl args' actual location in the ipgm code.
		}
//Atom::Trace(pgm_code,getObject()->get_reference(0)->code_size());
	}

	void	InputLessPGMOverlay::patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index){	//	patch recursively : in pgm_code[index] with IPGM_PTRs until ::.

		uint16	atom_count=pgm_code[pgm_code_index].getAtomCount();
		for(uint16	j=1;j<=atom_count;++j){

			switch(pgm_code[pgm_code_index+j].getDescriptor()){
			case	Atom::WILDCARD:
				pgm_code[pgm_code_index+j]=Atom::IPGMPointer(ipgm_code_index+j);
				break;
			case	Atom::T_WILDCARD:	//	leave as is and stop patching.
				return;
			case	Atom::I_PTR:
				patch_tpl_code(pgm_code[pgm_code_index+j].asIndex(),getObject()->code(ipgm_code_index+j).asIndex());
				break;
			default:	//	leave as is.
				break;
			}
		}
	}

	void	InputLessPGMOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index){
	}

	bool	InputLessPGMOverlay::inject_productions(_Mem	*mem,Controller	*origin){

		uint64	now=Now();

		uint16	unused_index;
		bool	in_red=false;	//	if prods are computed by red, we have to evaluate the expression; otherwise, we have to evaluate the prods in the set one by one to be able to reference new objects in this->productions.
		Context	prods(getObject()->get_reference(0),getView(),pgm_code,pgm_code[PGM_PRODS].asIndex(),this);
		if(prods[0].getDescriptor()!=Atom::SET){	//	prods[0] is not a set: it is assumed to be an expression lead by red.

			in_red=true;
			if(!prods.evaluate(unused_index)){

				rollback();
				productions.clear();
				return	false;
			}
			prods=*prods;//prods.trace();
		}

		uint16	production_count=prods.getChildrenCount();
		uint16	inj_eje_count=0;
		for(uint16	i=1;i<=production_count;++i){

			Context	cmd=*prods.getChild(i);
			if(!in_red	&&	!cmd.evaluate(unused_index)){

				rollback();
				productions.clear();
				return	false;
			}//cmd.trace();
			Context	function=*cmd.getChild(1);
			Context	device=*cmd.getChild(2);

			//	layout of a command:
			//	0	>cmd opcode
			//	1	>function
			//	2	>device
			//	3	>iptr to the set of arguments
			//	4	>set
			//	5	>first arg

			//	identify the production of new objects.
			Context	args=*cmd.getChild(4);
			if(device[0].atom==EXECUTIVE_DEVICE){

				if(function[0].asOpcode()==Opcodes::Inject	||
					function[0].asOpcode()==Opcodes::Eject){	//	args:[object view]; create an object if not a reference.

					Code	*object;
					Context	arg1=args.getChild(1);
					uint16	index=arg1.getIndex();
					arg1=*arg1;
					//arg1.trace();
					if(arg1.is_reference())
						productions.push_back(arg1.getObject());
					else{

						object=mem->buildObject(arg1[0]);
	//					if(production_count==2)
	//						arg1.trace();
						arg1.copy(object,0);
						//arg1.trace();
	//					if(production_count==2)
	//						object->trace();
						productions.push_back(mem->check_existence(object));
					}
					patch_code(index,Atom::ProductionPointer(productions.size()-1));

					++inj_eje_count;
				}
			}
		}

		Code	*mk_rdx;
		uint16	write_index;
		uint16	extent_index;
		bool	notify_rdx=inj_eje_count	&&	(getObject()->code(IPGM_NFR).asFloat()==1);
		if(notify_rdx){	//	the productions are command objects (cmd); only injections/ejections are notified.

			mk_rdx=get_mk_rdx(write_index);
			mk_rdx->code(write_index++)=Atom::Set(inj_eje_count);
			extent_index=write_index+inj_eje_count;
		}
		
		for(uint16	i=1;i<=production_count;++i){	//	all productions have evaluated correctly; now we can execute the commands.

			Context	cmd=*prods.getChild(i);
			Context	function=*cmd.getChild(1);
			Context	device=*cmd.getChild(2);

			//	call device functions.
			Context	args=*cmd.getChild(4);
			if(device[0].atom==EXECUTIVE_DEVICE){

				if(function[0].asOpcode()==Opcodes::Inject){	//	args:[object view]; retrieve the object and create a view.

					Context	arg1=args.getChild(1);
					arg1.dereference_once();
//arg1.trace();
					uint16	prod_index=arg1[0].asIndex();	//	args[1] is a prod_ptr.

					Code	*object=(*args.getChild(1)).getObject();
//object->trace();
					Context	_view=*args.getChild(2);
					View	*view=new	View();
					_view.copy(view,0);
					view->set_object(object);

					view->references[1]=getView()->get_host();
					view->code(VIEW_ORG)=Atom::RPointer(1);

					mem->inject(view);

					if(notify_rdx){

						mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
						(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
					}
				}else	if(function[0].asOpcode()==Opcodes::Eject){	//	args:[object view destination_node]; view.grp=destination grp (stdin ot stdout); retrieve the object and create a view.

					Code	*object=(*args.getChild(1)).getObject();
					
					Context	_view=*args.getChild(2);
					View	*view=new	View();
					_view.copy(view,0);
					view->set_object(object);

					Context	node=*args.getChild(3);

					mem->eject(view,node[0].getNodeID());

					if(notify_rdx){

						mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
						(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
					}
				}else	if(function[0].asOpcode()==Opcodes::Mod){	//	args:[iptr-to-cptr value].

					void				*object;
					Context::ObjectType	object_type;
					int16				member_index;
					uint32				view_oid;
					args.getChild(1).getMember(object,view_oid,object_type,member_index);	//	args.getChild(1) is an iptr.

					if(object){
						
						float32	value=(*args.getChild(2))[0].asFloat();
						switch(object_type){
						case	Context::TYPE_VIEW:{	//	add the target and value to the group's pending operations.

							Group	*g=(Group	*)object;
							g->enter();
							g->pending_operations.push_back(new	Group::Mod(view_oid,member_index,value));
							g->leave();
							break;
						}case	Context::TYPE_OBJECT:
							((Code	*)object)->mod(member_index,value);	//	protected internally.
							break;
						case	Context::TYPE_GROUP:
							((Group	*)object)->enter();
							((Group	*)object)->mod(member_index,value);
							((Group	*)object)->leave();
							break;
						default:
							rollback();
							productions.clear();
							return	false;
						}
					}
				}else	if(function[0].asOpcode()==Opcodes::Set){	//	args:[iptr-to-cptr value].

					void				*object;
					Context::ObjectType	object_type;
					int16				member_index;
					uint32				view_oid;
					args.getChild(1).getMember(object,view_oid,object_type,member_index);	//	args.getChild(1) is an iptr.

					if(object){
						
						float32	value=(*args.getChild(2))[0].asFloat();
						switch(object_type){
						case	Context::TYPE_VIEW:{	//	add the target and value to the group's pending operations.

							Group	*g=(Group	*)object;
							g->enter();
							g->pending_operations.push_back(new	Group::Set(view_oid,member_index,value));
							g->leave();
							break;
						}case	Context::TYPE_OBJECT:
							((Code	*)object)->set(member_index,value);	//	protected internally.
							break;
						case	Context::TYPE_GROUP:
							((Group	*)object)->enter();
							((Group	*)object)->set(member_index,value);
							((Group	*)object)->leave();
							break;
						}
					}
				}else	if(function[0].asOpcode()==Opcodes::NewClass){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::DelClass){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::LDC){		// TODO

				}else	if(function[0].asOpcode()==Opcodes::Swap){		// TODO

				}else	if(function[0].asOpcode()==Opcodes::NewDev){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::DelDev){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::Suspend){	//	no args.

					mem->suspend();
				}else	if(function[0].asOpcode()==Opcodes::Stop){		//	no args.

					mem->stop();
				}else{	//	unknown function.

					rollback();
					productions.clear();
					return	false;
				}
			}else{	//	in case of an external device, create a cmd object and send it.

				Code	*command=controller->get_mem()->buildObject(cmd[0]);
				cmd.copy(command,0);

				mem->eject(command,command->code(CMD_DEVICE).getNodeID());
			}
		}

		if(notify_rdx){

			uint16	ntf_grp_count=getView()->get_host()->get_ntf_grp_count();
			for(uint16	i=1;i<=ntf_grp_count;++i){

				NotificationView	*v=new	NotificationView(getView()->get_host(),getView()->get_host()->get_ntf_grp(i),mk_rdx);
				mem->injectNotificationNow(v,true,origin);
			}
		}

		return	true;
	}

	Code	*InputLessPGMOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(controller->get_mem());

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getObject());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	inputs.
		mk_rdx->code(extent_index++)=Atom::Set(0);
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					//	psln_thr.

		return	mk_rdx;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PGMOverlay::PGMOverlay(Controller	*c):InputLessPGMOverlay(c){

		init();
	}

	PGMOverlay::PGMOverlay(PGMOverlay	*original,uint16	last_input_index,uint16	value_commit_index):InputLessPGMOverlay(){

		controller=original->controller;

		input_pattern_indices=original->input_pattern_indices;
		input_pattern_indices.push_back(last_input_index);		//	put back the last original's input index.
		for(uint16	i=0;i<original->input_views.size()-1;++i)	//	ommit the last original's input view.
			input_views.push_back(original->input_views[i]);

		pgm_code_size=original->pgm_code_size;
		pgm_code=new	r_code::Atom[pgm_code_size];
		memcpy(pgm_code,original->pgm_code,pgm_code_size*sizeof(r_code::Atom));	//	copy patched code.

		Atom	*original_code=&getObject()->get_reference(0)->code(0);
		for(uint16	i=0;i<original->patch_indices.size();++i)	//	unpatch code.
			pgm_code[original->patch_indices[i]]=original_code[original->patch_indices[i]];

		this->value_commit_index=value_commit_index;
		for(uint16	i=0;i<value_commit_index;++i)	//	copy values up to the last commit index.
			values.push_back(original->values[i]);

		birth_time=Now();
	}

	inline	PGMOverlay::~PGMOverlay(){
	}

	inline	void	PGMOverlay::init(){

		//	init the list of pattern indices.
		uint16	pattern_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+1].asIndex();
		uint16	pattern_count=pgm_code[pattern_set_index].getAtomCount();
		for(uint16	i=1;i<=pattern_count;++i)
			input_pattern_indices.push_back(pgm_code[pattern_set_index+i].asIndex());

		birth_time=Now();
	}

	inline	void	PGMOverlay::reset(){

		InputLessPGMOverlay::reset();
		patch_indices.clear();
		input_views.clear();
		init();
	}

	void	PGMOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index){	//	patch recursively : in pgm_code[pgm_code_index] with IN_OBJ_PTRs until ::.

		uint16	patch_index=pgm_code_index;
		uint16	atom_count;
		if(pgm_code[pgm_code_index].getDescriptor()==Atom::I_PTR)
			patch_index=pgm_code[pgm_code_index].asIndex();	
		atom_count=pgm_code[patch_index].getAtomCount();

		pgm_code[pgm_code_index]=Atom::InObjPointer(input_index,input_code_index);	//	replace the skeleton atom by a ptr to the input object.
		patch_indices.push_back(pgm_code_index);

		for(uint16	j=1;j<=atom_count;++j){

			switch(pgm_code[patch_index+j].getDescriptor()){
			case	Atom::WILDCARD:
				pgm_code[patch_index+j]=Atom::InObjPointer(input_index,input_code_index+j);
				patch_indices.push_back(patch_index+j);
				break;
			case	Atom::T_WILDCARD:	//	leave as is and stop patching.
				return;
			case	Atom::I_PTR:	//	go one level deper in the pattern: recurse.
				patch_input_code(patch_index+j,input_index,getInputObject(input_index)->code(j).asIndex());
				patch_indices.push_back(patch_index+j);
				break;
			default:	//	leave as is.
				break;
			}
		}
	}

	void	PGMOverlay::reduce(r_exec::View	*input,_Mem	*mem){

		reductionCS.enter();

		if(alive){
			
			uint16	input_index;
			switch(match(input,input_index)){
			case	SUCCESS:
				if(input_pattern_indices.size()==0){	//	all patterns matched.

					if(check_timings()	&&	check_guards()	&&	inject_productions(mem,NULL)){

						((PGMController	*)controller)->remove(this);
						break;
					}
				}else{	//	create an overlay in a state where the last input is not matched: this overlay will be able to catch other candidates for the input patterns that have already been matched.

					PGMOverlay	*offspring=new	PGMOverlay(this,input_index,value_commit_index);
					((PGMController	*)controller)->add(offspring);
					commit();
					break;
				}
			case	FAILURE:	//	just rollback: let the overlay match other inputs.
				rollback();
				break;
			}
		}

		reductionCS.leave();
	}

	PGMOverlay::MatchResult	PGMOverlay::match(r_exec::View	*input,uint16	&input_index){

		input_views.push_back(input);
		bool	failed=false;
		std::list<uint16>::iterator	it;
		for(it=input_pattern_indices.begin();it!=input_pattern_indices.end();++it){

			MatchResult	r=_match(input,*it);
			switch(r){
			case	SUCCESS:
				input_index=*it;
				input_pattern_indices.erase(it);
				return	r;
			case	FAILURE:
				failed=true;
				rollback();	//	to try another pattern on a clean basis.
			case	IMPOSSIBLE:
				break;
			}
		}
		input_views.pop_back();
		return	failed?FAILURE:IMPOSSIBLE;
	}

	inline	PGMOverlay::MatchResult	PGMOverlay::_match(r_exec::View	*input,uint16	pattern_index){

		if(pgm_code[pattern_index].asOpcode()==Opcodes::AntiPTN){

			Context	input_object=Context::GetContextFromInput(input,this);
			Context	pattern_skeleton(getObject()->get_reference(0),getView(),pgm_code,pgm_code[pattern_index+1].asIndex(),this);	//	pgm_code[pattern_index] is the first atom of the pattern; pgm_code[pattern_index+1] is an iptr to the skeleton.
			if(!pattern_skeleton.match(input_object))
				return	SUCCESS;
			MatchResult	r=__match(input,pattern_index);
			switch(r){
			case	IMPOSSIBLE:
			case	FAILURE:
				return	SUCCESS;
			case	SUCCESS:
				return	FAILURE;
			}
		}else	if(pgm_code[pattern_index].asOpcode()==Opcodes::PTN){

			Context	input_object=Context::GetContextFromInput(input,this);
			Context	pattern_skeleton(getObject()->get_reference(0),getView(),pgm_code,pgm_code[pattern_index+1].asIndex(),this);	//	pgm_code[pattern_index] is the first atom of the pattern; pgm_code[pattern_index+1] is an iptr to the skeleton.
			if(!pattern_skeleton.match(input_object))
				return	IMPOSSIBLE;
			return	__match(input,pattern_index);
		}
		return	IMPOSSIBLE;
	}

	inline	PGMOverlay::MatchResult	PGMOverlay::__match(r_exec::View	*input,uint16	pattern_index){
//Atom::Trace(pgm_code,getObject()->get_reference(0)->code_size());
//input->object->trace();
		patch_input_code(pgm_code[pattern_index+1].asIndex(),input_views.size()-1,0);	//	the input has just been pushed on input_views (see match); pgm_code[pattern_index+1].asIndex() is the structure pointed by the pattern's skeleton.
//Atom::Trace(pgm_code,getObject()->get_reference(0)->code_size());
//input->object->trace();
		//	match: evaluate the set of guards.
		uint16	guard_set_index=pgm_code[pattern_index+2].asIndex();
		if(!evaluate(guard_set_index))
			return	FAILURE;
		return	SUCCESS;
	}

	bool	PGMOverlay::check_timings(){

		uint16	timing_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+2].asIndex();
		uint16	timing_count=pgm_code[timing_set_index].getAtomCount();
		for(uint16	i=1;i<=timing_count;++i)
			if(!evaluate(timing_set_index+i))
				return	false;
		return	true;
	}

	bool	PGMOverlay::check_guards(){

		uint16	guard_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+3].asIndex();
		uint16	guard_count=pgm_code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i)
			if(!evaluate(guard_set_index+i))
				return	false;
		return	true;
	}

	Code	*PGMOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(controller->get_mem());

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getObject());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	inputs.
		mk_rdx->code(extent_index++)=Atom::Set(input_views.size());
		for(uint16	i=0;i<input_views.size();++i){

			mk_rdx->code(extent_index++)=Atom::RPointer(i+1);
			mk_rdx->add_reference(input_views[i]->object);
		}
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					//	psln_thr.

		return	mk_rdx;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	AntiPGMOverlay::AntiPGMOverlay(Controller	*c):PGMOverlay(c){
	}

	inline	AntiPGMOverlay::AntiPGMOverlay(AntiPGMOverlay	*original,uint16	last_input_index,uint16	value_limit):PGMOverlay(original,last_input_index,value_limit){
	}

	inline	AntiPGMOverlay::~AntiPGMOverlay(){
	}

	void	AntiPGMOverlay::reduce(r_exec::View	*input,_Mem	*mem){

		reductionCS.enter();

		if(alive){

			uint16	input_index;
			switch(match(input,input_index)){
			case	SUCCESS:
				if(input_pattern_indices.size()==0){	//	all patterns matched.

					if(check_timings()	&&	check_guards()){

						((AntiPGMController	*)controller)->restart(this);
						break;
					}
				}else{

					AntiPGMOverlay	*offspring=new	AntiPGMOverlay(this,input_index,value_commit_index);
					((AntiPGMController	*)controller)->add(offspring);
					commit();
					break;
				}
			case	FAILURE:	//	just rollback: let the overlay match other inputs.
				rollback();
				break;
			}
		}

		reductionCS.leave();
	}

	Code	*AntiPGMOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_ANTI_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(controller->get_mem());

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkAntiRdx,MK_ANTI_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getObject());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					//	psln_thr.

		return	mk_rdx;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	InputLessPGMController::InputLessPGMController(_Mem	*m,r_code::View	*ipgm_view):Controller(m,ipgm_view){

		overlays.push_back(new	InputLessPGMOverlay(this));
	}
	
	InputLessPGMController::~InputLessPGMController(){
	}

	void	InputLessPGMController::signal_input_less_pgm(){	//	next job will be pushed by the rMem upon processing the current signaling job, i.e. right after exiting this function.

		overlayCS.enter();
		if(overlays.size()){

			Overlay	*o=*overlays.begin();
			o->inject_productions(mem,NULL);
			o->reset();

			Group	*host=getView()->get_host();
			host->enter();
			if(getView()->get_act_vis()>host->get_act_thr()	&&	//	active ipgm.
				host->get_c_act()>host->get_c_act_thr()			&&	//	c-active group.
				host->get_c_sln()>host->get_c_sln_thr()){			//	c-salient group.

				TimeJob	*next_job=new	InputLessPGMSignalingJob(this,Now()+tsc);
				mem->pushTimeJob(next_job);
			}
			host->leave();
		}
		overlayCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PGMController::PGMController(_Mem	*m,r_code::View	*ipgm_view):Controller(m,ipgm_view){

		overlays.push_back(new	PGMOverlay(this));
	}
	
	PGMController::~PGMController(){
	}

	void	PGMController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		overlayCS.enter();

		if(tsc>0){	// 1st overlay is the master (no match yet); other overlays are pushed back in order of their matching time. 
			
			// start from the last overlay, and erase all of them that are older than tsc.
			uint64	now=Now();
			std::list<P<Overlay> >::iterator	master=overlays.begin();
			std::list<P<Overlay> >::iterator	o;
			std::list<P<Overlay> >::iterator	previous;
			for(o=overlays.end();o!=master;){

				if(now-((PGMOverlay	*)(*o))->birth_time>tsc){
					
					previous=--o;
					(*o)->kill();
					overlays.erase(o);
					o=previous;
				}else
					break;
			}
		}

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob	*j=new	ReductionJob(new	View(input),*o);
			mem->pushReductionJob(j);
		}

		overlayCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AntiPGMController::AntiPGMController(_Mem	*m,r_code::View	*ipgm_view):Controller(m,ipgm_view),successful_match(false){

		overlays.push_back(new	AntiPGMOverlay(this));
	}
	
	AntiPGMController::~AntiPGMController(){
	}

	void	AntiPGMController::take_input(r_exec::View	*input,Controller	*origin){

		if(this!=origin)
			overlayCS.enter();

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob	*j=new	ReductionJob(new	View(input),*o);
			mem->pushReductionJob(j);
		}

		if(this!=origin)
			overlayCS.leave();
	}

	void	AntiPGMController::signal_anti_pgm(){

		overlayCS.enter();

		AntiPGMOverlay	*overlay=(AntiPGMOverlay	*)*overlays.begin();
		overlay->reductionCS.enter();
		if(!successful_match)
			overlay->inject_productions(mem,this);	//	eventually calls take_input(): origin set to this to avoid a deadlock on overlayCS.
		overlay->reset();
		overlay->reductionCS.leave();
		
		push_new_signaling_job();

		std::list<P<Overlay> >::const_iterator	first=overlays.begin();
		std::list<P<Overlay> >::const_iterator	o;
		for(o=++first;o!=overlays.end();){	//	reset the first overlay and kill all others.

			(*o)->kill();
			o=overlays.erase(o);
		}

		successful_match=false;

		overlayCS.leave();
	}

	void	AntiPGMController::restart(AntiPGMOverlay	*overlay){	//	one anti overlay matched all its inputs, timings and guards: push a new signaling job, 
																	//	reset the overlay and kill all others.
		overlayCS.enter();
		
		overlay->reset();
		
		push_new_signaling_job();

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();){

			if(overlay!=*o){

				((AntiPGMOverlay	*)*o)->kill();
				o=overlays.erase(o);
			}else
				++o;
		}

		successful_match=true;

		overlayCS.leave();
	}

	void	AntiPGMController::push_new_signaling_job(){

		Group	*host=getView()->get_host();
		host->enter();
		if(getView()->get_act_vis()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()	&&			//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){			//	c-salient group.

				host->leave();
			TimeJob	*next_job=new	AntiPGMSignalingJob(this,Now()+Utils::GetTimestamp<Code>(getObject(),IPGM_TSC));
			mem->pushTimeJob(next_job);
		}else
			host->leave();
	}
}