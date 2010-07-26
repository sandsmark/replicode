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

	Overlay::Overlay():alive(true){	//	used for constructing IOverlay offsprings.
	}

	Overlay::Overlay(Controller	*c):_Object(),controller(c),alive(true),value_commit_index(0),_now(0){

		//	copy the original pgm code.
		pgm_code_size=getIPGM()->get_reference(0)->code_size();
		pgm_code=new	r_code::Atom[pgm_code_size];
		memcpy(pgm_code,&getIPGM()->get_reference(0)->code(0),pgm_code_size*sizeof(r_code::Atom));
		patch_tpl_args();
	}

	Overlay::~Overlay(){

		delete[]	pgm_code;
	}

	inline	void	Overlay::reset(){

		memcpy(pgm_code,&getIPGM()->get_reference(0)->code(0),pgm_code_size*sizeof(r_code::Atom));	//	restore code to prisitne copy.
		patch_tpl_args();

		patch_indices.clear();
		value_commit_index=0;
		values.as_std()->clear();
		productions.clear();
	}

	r_code::Code	*Overlay::buildObject(Atom	head)	const{
		
		return	controller->get_mem()->buildObject(head);
	}

	inline	void	Overlay::rollback(){

		Atom	*original_code=&getIPGM()->get_reference(0)->code(0);
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

	inline	void	Overlay::commit(){

		patch_indices.clear();
		value_commit_index=values.size();
	}

	inline	bool	Overlay::evaluate(uint16	index){

		Context	c(getIPGM()->get_reference(0),getIPGMView(),pgm_code,index,this);
		uint16	result_index;
		return	c.evaluate(result_index);
	}

	void	Overlay::patch_tpl_args(){	//	no rollback on that part of the code.
//getIPGM()->trace();
		uint16	tpl_arg_set_index=pgm_code[PGM_TPL_ARGS].asIndex();			//	index to the set of all tpl patterns.
		uint16	arg_count=pgm_code[tpl_arg_set_index].getAtomCount();
		uint16	ipgm_arg_set_index=getIPGM()->code(IPGM_ARGS).asIndex();	//	index to the set of all ipgm tpl args.
		for(uint16	i=1;i<=arg_count;++i){									//	pgm_code[tpl_arg_set_index+i] is an iptr to a pattern.

			Atom	&skel_iptr=pgm_code[pgm_code[tpl_arg_set_index+i].asIndex()+1];
			patch_tpl_code(skel_iptr.asIndex(),getIPGM()->code(ipgm_arg_set_index+i).asIndex());
			skel_iptr=Atom::IPGMPointer(ipgm_arg_set_index+i);				//	patch the pgm code with ptrs to the tpl args' actual location in the ipgm code.
		}
//Atom::Trace(pgm_code,getIPGM()->get_reference(0)->code_size());
	}

	void	Overlay::patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index){	//	patch recursively : in pgm_code[index] with IPGM_PTRs until ::.

		uint16	atom_count=pgm_code[pgm_code_index].getAtomCount();
		for(uint16	j=1;j<=atom_count;++j){

			switch(pgm_code[pgm_code_index+j].getDescriptor()){
			case	Atom::WILDCARD:
				pgm_code[pgm_code_index+j]=Atom::IPGMPointer(ipgm_code_index+j);
				break;
			case	Atom::T_WILDCARD:	//	leave as is and stop patching.
				return;
			case	Atom::I_PTR:
				patch_tpl_code(pgm_code[pgm_code_index+j].asIndex(),getIPGM()->code(ipgm_code_index+j).asIndex());
				break;
			default:	//	leave as is.
				break;
			}
		}
	}

	void	Overlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index){
	}

	bool	Overlay::inject_productions(_Mem	*mem,_PGMController	*origin){

		_now=Now();

		uint16	unused_index;
		bool	in_red=false;	//	if prods are computed by red, we have to evaluate the expression; otherwise, we have to evaluate the prods in the set one by one to be able to reference new objects in this->productions.
		Context	prods(getIPGM()->get_reference(0),getIPGMView(),pgm_code,pgm_code[PGM_PRODS].asIndex(),this);
		if(prods[0].getDescriptor()!=Atom::SET){	//	p points to an expression lead by red.

			in_red=true;
			if(!prods.evaluate(unused_index)){

				rollback();
				productions.clear();
				_now=0;
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
				_now=0;
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

						object=controller->get_mem()->buildObject(arg1[0]);
						arg1.copy(object,0);
						productions.push_back(object);
					}
					patch_code(index,Atom::ProductionPointer(productions.size()-1));

					++inj_eje_count;
				}
			}
		}

		Code	*mk_rdx;
		uint16	write_index;
		uint16	extent_index;
		bool	notify_rdx=(getIPGM()->get_reference(0)->code(PGM_NFR).asFloat()==1)	&&	inj_eje_count;
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

					uint16	prod_index=arg1[0].asIndex();	//	args[1] is a prod_ptr.

					Code	*object=(*args.getChild(1)).getObject();
					
					Context	_view=*args.getChild(2);
					View	*view=new	View();
					_view.copy(view,0);
					view->set_object(object);

					view->references[1]=getIPGMView()->get_host();
					view->code(VIEW_ORG)=Atom::RPointer(1);

					Code	*existing_object=mem->inject(view);
					if(existing_object)
						productions[prod_index]=existing_object;	//	so that the mk.rdx will reference the existing object instead of object, which has been discarded.

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
							_now=0;
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
				}else	if(function[0].asOpcode()==Opcodes::NewClass){

				}else	if(function[0].asOpcode()==Opcodes::DelClass){

				}else	if(function[0].asOpcode()==Opcodes::LDC){

				}else	if(function[0].asOpcode()==Opcodes::Swap){

				}else	if(function[0].asOpcode()==Opcodes::NewDev){

				}else	if(function[0].asOpcode()==Opcodes::DelDev){

				}else	if(function[0].asOpcode()==Opcodes::Stop){		//	no args.

					mem->stop();
				}else{	//	unknown function.

					rollback();
					productions.clear();
					_now=0;
					return	false;
				}
			}else{	//	in case of an external device, create a cmd object and send it.

				Code	*command=controller->get_mem()->buildObject(cmd[0]);
				cmd.copy(command,0);

				mem->eject(command,command->code(CMD_DEVICE).getNodeID());
			}
		}

		if(notify_rdx){

			NotificationView	*v=new	NotificationView(getIPGMView()->get_host(),getIPGMView()->get_host()->get_ntf_grp(),mk_rdx);
			mem->injectNotificationNow(v,true,origin);
		}

		_now=0;
		return	true;
	}

	Code	*Overlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(controller->get_mem());//controller->get_mem()->buildObject(Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY));

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getIPGM());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	inputs.
		mk_rdx->code(extent_index++)=Atom::Set(0);
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					//	psln_thr.

		return	mk_rdx;
	}

	uint64	Overlay::now()	const{

		if(_now>0)
			return	_now;
		return	Now();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	IOverlay::IOverlay(Controller	*c):Overlay(c){

		init();
	}

	IOverlay::IOverlay(PGMController	*c):Overlay(c){

		init();
	}

	IOverlay::IOverlay(IOverlay	*original,uint16	last_input_index,uint16	value_commit_index):Overlay(){

		controller=original->controller;

		input_pattern_indices=original->input_pattern_indices;
		input_pattern_indices.push_back(last_input_index);		//	put back the last original's input index.
		for(uint16	i=0;i<original->input_views.size()-1;++i)	//	ommit the last original's input view.
			input_views.push_back(original->input_views[i]);

		pgm_code_size=original->pgm_code_size;
		pgm_code=new	r_code::Atom[pgm_code_size];
		memcpy(pgm_code,original->pgm_code,pgm_code_size*sizeof(r_code::Atom));	//	copy patched code.

		Atom	*original_code=&getIPGM()->get_reference(0)->code(0);
		for(uint16	i=0;i<original->patch_indices.size();++i)	//	unpatch code.
			pgm_code[original->patch_indices[i]]=original_code[original->patch_indices[i]];

		this->value_commit_index=value_commit_index;
		for(uint16	i=0;i<value_commit_index;++i)	//	copy values up to the last commit index.
			values.push_back(original->values[i]);
	}

	inline	IOverlay::~IOverlay(){
	}

	inline	void	IOverlay::init(){

		//	init the list of pattern indices.
		uint16	pattern_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+1].asIndex();
		uint16	pattern_count=pgm_code[pattern_set_index].getAtomCount();
		for(uint16	i=1;i<=pattern_count;++i)
			input_pattern_indices.push_back(pgm_code[pattern_set_index+i].asIndex());

	}

	inline	void	IOverlay::reset(){

		Overlay::reset();
		patch_indices.clear();
		input_views.clear();
	}

	void	IOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index){	//	patch recursively : in pgm_code[index] with IN_OBJ_PTRs until ::.

		uint16	skel_index=pgm_code[pgm_code_index+1].asIndex();
		uint16	atom_count=pgm_code[skel_index].getAtomCount();

		pgm_code[skel_index]=Atom::InObjPointer(input_index,0);	//	replace the skeleton atom by a ptr to the input object.
		patch_indices.push_back(skel_index);

		for(uint16	j=1;j<=atom_count;++j){

			switch(pgm_code[skel_index+j].getDescriptor()){
			case	Atom::WILDCARD:
				pgm_code[skel_index+j]=Atom::InObjPointer(input_index,input_code_index+j);
				patch_indices.push_back(skel_index+j);
				break;
			case	Atom::T_WILDCARD:	//	leave as is and stop patching.
				return;
			case	Atom::I_PTR:
				patch_input_code(pgm_code[skel_index+j].asIndex(),input_index,getInputObject(input_index)->code(skel_index+j).asIndex());
				patch_indices.push_back(skel_index+j);
				break;
			default:	//	leave as is.
				break;
			}
		}
	}

	void	IOverlay::reduce(r_exec::View	*input,_Mem	*mem){

		reductionCS.enter();

		uint16	input_index;
		switch(match(input,input_index)){
		case	SUCCESS:
			if(input_pattern_indices.size()==0){	//	all patterns matched.

				if(check_timings()	&&	check_guards()	&&	inject_productions(mem,NULL)){

					((PGMController	*)controller)->remove(this);
					break;
				}
			}else{	//	create an overlay in a state where the last input is not matched: this overlay will be able to catch other candidates for the input patterns that have already been matched.

				IOverlay	*offspring=new	IOverlay(this,input_index,value_commit_index);
				((PGMController	*)controller)->add(offspring);
				commit();
				break;
			}
		case	FAILURE:	//	just rollback: let the overlay match other inputs.
			rollback();
			break;
		}

		reductionCS.leave();
	}

	IOverlay::MatchResult	IOverlay::match(r_exec::View	*input,uint16	&input_index){

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

	inline	IOverlay::MatchResult	IOverlay::_match(r_exec::View	*input,uint16	pattern_index){

		if(pgm_code[pattern_index].asOpcode()==Opcodes::AntiPTN){

			Context	input_object=Context::GetContextFromInput(input,this);
			Context	pattern_skeleton(getIPGM()->get_reference(0),getIPGMView(),pgm_code,pgm_code[pattern_index+1].asIndex(),this);	//	pgm_code[pattern_index] is the first atom of the pattern; pgm_code[pattern_index+1] is an iptr to the skeleton.
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
			Context	pattern_skeleton(getIPGM()->get_reference(0),getIPGMView(),pgm_code,pgm_code[pattern_index+1].asIndex(),this);	//	pgm_code[pattern_index] is the first atom of the pattern; pgm_code[pattern_index+1] is an iptr to the skeleton.
			if(!pattern_skeleton.match(input_object))
				return	IMPOSSIBLE;
			return	__match(input,pattern_index);
		}
		return	IMPOSSIBLE;
	}

	inline	IOverlay::MatchResult	IOverlay::__match(r_exec::View	*input,uint16	pattern_index){

		patch_input_code(pattern_index,input_views.size()-1,0);	//	the input has just been pushed on input_views (see match).
//Atom::Trace(pgm_code,getIPGM()->get_reference(0)->code_size());
//input->object->trace();
		//	match: evaluate the set of guards.
		uint16	guard_set_index=pgm_code[pattern_index+2].asIndex();
		if(!evaluate(guard_set_index))
			return	FAILURE;
		return	SUCCESS;
	}

	bool	IOverlay::check_timings(){

		uint16	timing_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+2].asIndex();
		uint16	timing_count=pgm_code[timing_set_index].getAtomCount();
		for(uint16	i=1;i<=timing_count;++i)
			if(!evaluate(timing_set_index+i))
				return	false;
		return	true;
	}

	bool	IOverlay::check_guards(){

		uint16	guard_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+3].asIndex();
		uint16	guard_count=pgm_code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i)
			if(!evaluate(guard_set_index+i))
				return	false;
		return	true;
	}

	Code	*IOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(controller->get_mem());//controller->get_mem()->buildObject(Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY));

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getIPGM());
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

	inline	AntiOverlay::AntiOverlay(AntiPGMController	*c):IOverlay(c){
	}

	inline	AntiOverlay::AntiOverlay(AntiOverlay	*original,uint16	last_input_index,uint16	value_limit):IOverlay(original,last_input_index,value_limit){
	}

	inline	AntiOverlay::~AntiOverlay(){
	}

	void	AntiOverlay::reduce(r_exec::View	*input,_Mem	*mem){

		reductionCS.enter();

		uint16	input_index;
		switch(match(input,input_index)){
		case	SUCCESS:
			if(input_pattern_indices.size()==0){	//	all patterns matched.

				if(check_timings()	&&	check_guards()){

					((AntiPGMController	*)controller)->restart(this);
					break;
				}
			}else{

				AntiOverlay	*offspring=new	AntiOverlay(this,input_index,value_commit_index);
				((AntiPGMController	*)controller)->add(offspring);
				commit();
				break;
			}
		case	FAILURE:	//	just rollback: let the overlay match other inputs.
			rollback();
			break;
		}

		reductionCS.leave();
	}

	Code	*AntiOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_ANTI_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(controller->get_mem());//controller->get_mem()->buildObject(Atom::Marker(Opcodes::MkAntiRdx,MK_ANTI_RDX_ARITY));

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkAntiRdx,MK_ANTI_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getIPGM());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					//	psln_thr.

		return	mk_rdx;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Controller::Controller(_Mem	*m,r_code::View	*ipgm_view):_Object(),mem(m),ipgm_view(ipgm_view),alive(true){
	}
	
	Controller::~Controller(){
	}

	void	Controller::kill(){
		
		aliveCS.enter();
		alive=false;
		aliveCS.leave();

		std::list<P<Overlay> >::const_iterator	o;
		overlayCS.enter();
		for(o=overlays.begin();o!=overlays.end();++o)
			(*o)->kill();
		overlays.clear();
		overlayCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	InputLessPGMController::InputLessPGMController(_Mem	*m,r_code::View	*ipgm_view):Controller(m,ipgm_view){

		overlays.push_back(new	Overlay(this));
	}
	
	InputLessPGMController::~InputLessPGMController(){
	}

	void	InputLessPGMController::signal_input_less_pgm(){	//	next job will be pushed by the rMem upon processing the current signaling job, i.e. right after exiting this function.

		Overlay	*o=*overlays.begin();
		o->inject_productions(mem,NULL);

		Group	*host=getIPGMView()->get_host();
		host->enter();
		if(getIPGMView()->get_act_vis()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()			&&	//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){			//	c-salient group.

			TimeJob	*next_job=new	InputLessPGMSignalingJob(this,Now()+Timestamp::Get<Code>(getIPGM()->get_reference(0),PGM_TSC));
			mem->pushTimeJob(next_job);
		}
		host->leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_PGMController::_PGMController(_Mem	*m,r_code::View	*ipgm_view):Controller(m,ipgm_view){
	}
	
	_PGMController::~_PGMController(){
	}

	inline	void	_PGMController::add(Overlay	*overlay){	//	o has just matched an input; builds a copy of o.

		overlayCS.enter();
		overlays.push_back(overlay);
		overlayCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PGMController::PGMController(_Mem	*m,r_code::View	*ipgm_view):_PGMController(m,ipgm_view),start_time(Now()){

		overlays.push_back(new	IOverlay(this));
	}
	
	PGMController::~PGMController(){
	}

	void	PGMController::take_input(r_exec::View	*input,_PGMController	*origin){	//	origin unused since there is no recursion here.

		overlayCS.enter();

		uint64	tsc=Timestamp::Get<Code>(getIPGM()->get_reference(0),PGM_TSC);
		if(tsc>0){
			
			uint64	now=Now();
			uint64	elapsed=now-start_time;
			if(elapsed>tsc){

				std::list<P<Overlay> >::const_iterator	first=overlays.begin();
				std::list<P<Overlay> >::const_iterator	o;
				for(o=++first;o!=overlays.end();){

					(*o)->kill();
					o=overlays.erase(o);
				}

				first=overlays.begin();
				((IOverlay	*)*first)->reductionCS.enter();
				(*first)->reset();
				((IOverlay	*)*first)->reductionCS.leave();
				start_time=elapsed%tsc;
			}
		}

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob	*j=new	ReductionJob(new	View(input),*o);
			mem->pushReductionJob(j);
		}

		overlayCS.leave();
	}

	inline	void	PGMController::remove(Overlay	*overlay){

		overlayCS.enter();
		if(overlays.size()>1){

			overlay->kill();
			overlays.remove(overlay);
		}else
			overlay->reset();
		overlayCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AntiPGMController::AntiPGMController(_Mem	*m,r_code::View	*ipgm_view):_PGMController(m,ipgm_view),successful_match(false){

		overlays.push_back(new	AntiOverlay(this));
	}
	
	AntiPGMController::~AntiPGMController(){
	}

	void	AntiPGMController::take_input(r_exec::View	*input,_PGMController	*origin){

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

		AntiOverlay	*overlay=(AntiOverlay	*)*overlays.begin();
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

	void	AntiPGMController::restart(AntiOverlay	*overlay){	//	one anti overlay matched all its inputs, timings and guards: push a new signaling job, 
																//	reset the overlay and kill all others.
		overlayCS.enter();
		
		overlay->reset();
		
		push_new_signaling_job();

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();){

			if(overlay!=*o){

				((AntiOverlay	*)*o)->kill();
				o=overlays.erase(o);
			}else
				++o;
		}

		successful_match=true;

		overlayCS.leave();
	}

	void	AntiPGMController::push_new_signaling_job(){

		Group	*host=getIPGMView()->get_host();
		host->enter();
		if(getIPGMView()->get_act_vis()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()	&&			//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){			//	c-salient group.

				host->leave();
			TimeJob	*next_job=new	AntiPGMSignalingJob(this,Now()+Timestamp::Get<Code>(getIPGM()->get_reference(0),PGM_TSC));
			mem->pushTimeJob(next_job);
		}else
			host->leave();
	}
}