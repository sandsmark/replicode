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
#include	"pgm_controller.h"
#include	"mem.h"
#include	"group.h"
#include	"opcodes.h"
#include	"context.h"
#include	"callbacks.h"


//	pgm layout:
//
//	index								content
//
//	PGM_TPL_ARGS						>iptr to the tpl args set
//	PGM_INPUTS							>iptr to the pattern set
//	PGM_GUARDS							>iptr to the guard set
//	PGM_PRODS							>iptr to the production set
//	pgm_code[PGM_TPL_ARGS]				>tpl arg set #n0
//	pgm_code[PGM_TPL_ARGS]+1			>iptr to first tpl pattern
//	...									>...
//	pgm_code[PGM_TPL_ARGS]+n0			>iptr to last tpl pattern
//	pgm_code[pgm_code[PGM_TPL_ARGS]+1]	>opcode of the first tpl pattern
//	...									>...
//	pgm_code[pgm_code[PGM_TPL_ARGS]+n0]	>opcode of the last tpl pattern
//	pgm_code[PGM_INPUTS]				>input pattern set #n1
//	pgm_code[PGM_INPUTS]+1				>iptr to first input pattern
//	...									>...
//	pgm_code[PGM_INPUTS]+n1				>iptr to last input pattern
//	pgm_code[pgm_code[PGM_INPUTS]+1]	>opcode of the first input pattern
//	...									>...
//	pgm_code[pgm_code[PGM_INPUTS]+n1]	>opcode of the last input pattern
//	...									>...
//	pgm_code[PGM_GUARDS]				>guard set #n2
//	pgm_code[PGM_GUARDS]+1				>iptr to first guard
//	...									>...
//	pgm_code[PGM_GUARDS]+n2				>iptr to last guard
//	pgm_code[pgm_code[PGM_GUARDS]+1]	>opcode of the first guard
//	...									>...
//	pgm_code[pgm_code[PGM_GUARDS]+n2]	>opcode of the last guard
//	...									>...
//	pgm_code[PGM_PRODS]					>production set #n3
//	pgm_code[PGM_PRODS]+1				>iptr to first production
//	...									>...
//	pgm_code[PGM_PRODS]+n3				>iptr to last production
//	pgm_code[pgm_code[PGM_PRODS]+1]		>opcode of the first production
//	...									>...
//	pgm_code[pgm_code[PGM_PRODS]+n3]	>opcode of the last production
//	...									>...

namespace	r_exec{

	InputLessPGMOverlay::InputLessPGMOverlay():Overlay(){	// used for constructing PGMOverlay offsprings.
	}

	InputLessPGMOverlay::InputLessPGMOverlay(Controller	*c):Overlay(c){

		patch_tpl_args();
	}

	InputLessPGMOverlay::~InputLessPGMOverlay(){
	}

	inline	void	InputLessPGMOverlay::reset(){

		Overlay::reset();

		patch_tpl_args();

		patch_indices.clear();
		value_commit_index=0;
		values.as_std()->clear();
		productions.clear();
	}

	inline	bool	InputLessPGMOverlay::evaluate(uint16	index){

		IPGMContext	c(getObject()->get_reference(0),getView(),code,index,this);
		uint16	result_index;
		return	c.evaluate(result_index);
	}

	void	InputLessPGMOverlay::patch_tpl_args(){	// no rollback on that part of the code.
//getObject()->trace();
		uint16	tpl_arg_set_index=code[PGM_TPL_ARGS].asIndex();			// index to the set of all tpl patterns.
		uint16	arg_count=code[tpl_arg_set_index].getAtomCount();
		uint16	ipgm_arg_set_index=getObject()->code(IPGM_ARGS).asIndex();	// index to the set of all ipgm tpl args.
		for(uint16	i=1;i<=arg_count;++i){									// pgm_code[tpl_arg_set_index+i] is an iptr to a pattern.

			Atom	&skel_iptr=code[code[tpl_arg_set_index+i].asIndex()+1];
			patch_tpl_code(skel_iptr.asIndex(),getObject()->code(ipgm_arg_set_index+i).asIndex());
			skel_iptr=Atom::IPGMPointer(ipgm_arg_set_index+i);				// patch the pgm code with ptrs to the tpl args' actual location in the ipgm code.
		}
//Atom::Trace(pgm_code,getObject()->get_reference(0)->code_size());
	}

	void	InputLessPGMOverlay::patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index){	// patch recursively : in pgm_code[index] with IPGM_PTRs until ::.

		uint16	atom_count=code[pgm_code_index].getAtomCount();
		for(uint16	j=1;j<=atom_count;++j){

			switch(code[pgm_code_index+j].getDescriptor()){
			case	Atom::WILDCARD:
				code[pgm_code_index+j]=Atom::IPGMPointer(ipgm_code_index+j);
				break;
			case	Atom::T_WILDCARD:	// leave as is and stop patching.
				return;
			case	Atom::I_PTR:
				patch_tpl_code(code[pgm_code_index+j].asIndex(),getObject()->code(ipgm_code_index+j).asIndex());
				break;
			default:	// leave as is.
				break;
			}
		}
	}

	void	InputLessPGMOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index,int16	parent_index){
	}

	bool	InputLessPGMOverlay::inject_productions(){

		uint64	now=Now();

		uint16	unused_index;
		bool	in_red=false;	// if prods are computed by red, we have to evaluate the expression; otherwise, we have to evaluate the prods in the set one by one to be able to reference new objects in this->productions.
		IPGMContext	prods(getObject()->get_reference(0),getView(),code,code[PGM_PRODS].asIndex(),this);
		if(prods[0].getDescriptor()!=Atom::SET){	// prods[0] is not a set: it is assumed to be an expression lead by red.

			in_red=true;
			if(!prods.evaluate(unused_index)){

				rollback();
				productions.clear();
				return	false;
			}
			prods=*prods;
		}
//prods.trace();
		uint16	production_count=prods.getChildrenCount();
		uint16	cmd_count=0;	// cmds to the executive (excl. mod/set) and external devices.
		for(uint16	i=1;i<=production_count;++i){

			IPGMContext	cmd=*prods.getChild(i);
			if(!in_red	&&	!cmd.evaluate(unused_index)){

				rollback();
				productions.clear();
				return	false;
			}//cmd.trace();
			IPGMContext	function=*cmd.getChild(1);

			//	layout of a command:
			//	0	>icmd opcode
			//	1	>function
			//	2	>iptr to the set of arguments
			//	3	>set
			//	4	>first arg
			//	or:
			//	0	>cmd opcode
			//	1	>function
			//	2	>iptr to the set of arguments
			//	3	>psln_thr
			//	4	>set
			//	5	>first arg

			//	identify the production of new objects.
			IPGMContext	args=*cmd.getChild(2);
			if(cmd[0].asOpcode()==Opcodes::ICmd){

				if(function[0].asOpcode()==Opcodes::Inject	||
					function[0].asOpcode()==Opcodes::Eject){	// args:[object view]; create an object if not a reference.

					Code	*object;
					IPGMContext	arg1=args.getChild(1);
					uint16	index=arg1.getIndex();
					arg1=*arg1;
//					arg1.trace();
					if(arg1.is_reference())
						productions.push_back(arg1.getObject());
					else{

						object=_Mem::Get()->build_object(arg1[0]);
//						arg1.trace();
						arg1.copy(object,0);
//						arg1.trace();
//						object->trace();
						productions.push_back(_Mem::Get()->check_existence(object));
					}
					patch_code(index,Atom::ProductionPointer(productions.size()-1));

					++cmd_count;
				}else	if(function[0].asOpcode()!=Opcodes::Mod		&&
							function[0].asOpcode()!=Opcodes::Set	&&
							function[0].asOpcode()!=Opcodes::Prb)
					++cmd_count;
			}else
				++cmd_count;
		}

		Code	*mk_rdx=NULL;
		uint16	ntf_grp_count=getView()->get_host()->get_ntf_grp_count();

		uint16	write_index;
		uint16	mk_rdx_prod_index;
		uint16	extent_index;
		if(ntf_grp_count	&&	cmd_count	&&	(getObject()->code(IPGM_NFR).asBoolean())){	// the productions are command objects (cmd); only injections/ejections and cmds to external devices are notified.

			mk_rdx=get_mk_rdx(write_index);
			mk_rdx_prod_index=write_index;
			mk_rdx->code(write_index++)=Atom::Set(cmd_count);
			extent_index=write_index+cmd_count;
		}

		// all productions have evaluated correctly; now we can execute the commands one by one.
		for(uint16	i=1;i<=production_count;++i){

			IPGMContext	cmd=*prods.getChild(i);
			IPGMContext	function=*cmd.getChild(1);

			// call device functions.
			IPGMContext	args=*cmd.getChild(2);
			if(cmd[0].asOpcode()==Opcodes::ICmd){	// command to the executive.

				if(function[0].asOpcode()==Opcodes::Inject){	// args:[object view]; retrieve the object and create a view.

					IPGMContext	arg1=args.getChild(1);
					arg1.dereference_once();
//arg1.trace();
					Code	*object=(*args.getChild(1)).getObject();
//object->trace();
					IPGMContext	_view=*args.getChild(2);
					if(_view[0].getAtomCount()!=0){	// regular view (i.e. not |[]).
					
						View	*view=new	View();
						_view.copy(view,0);
						view->set_object(object);

						view->references[1]=getView()->get_host();
						view->code(VIEW_ORG)=Atom::RPointer(1);

						_Mem::Get()->inject(view);

						if(mk_rdx){

							mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
							(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
						}
					}else	// this allows building objects with no view (case in point: fact on object: only the fact needs to be injected).
						--cmd_count;
				}else	if(function[0].asOpcode()==Opcodes::Eject){	// args:[object view destination_node]; view.grp=destination grp (stdin ot stdout); retrieve the object and create a view.

					Code	*object=(*args.getChild(1)).getObject();
					
					IPGMContext	_view=*args.getChild(2);
					View	*view=new	View();
					_view.copy(view,0);
					view->set_object(object);

					IPGMContext	node=*args.getChild(3);

					_Mem::Get()->eject(view,node[0].getNodeID());

					if(mk_rdx){

						mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
						(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
					}
				}else	if(function[0].asOpcode()==Opcodes::Mod){	// args:[iptr-to-cptr value].

					void				*object;
					IPGMContext::ObjectType	object_type;
					int16				member_index;
					uint32				view_oid;
					args.getChild(1).getMember(object,view_oid,object_type,member_index);	// args.getChild(1) is an iptr.

					if(object){

						float32	value=(*args.getChild(2))[0].asFloat();
						switch(object_type){
						case	IPGMContext::TYPE_VIEW:{	// add the target and value to the group's pending operations.

							Group	*g=(Group	*)object;
							g->enter();
							g->pending_operations.push_back(new	Group::Mod(view_oid,member_index,value));
							g->leave();
							break;
						}case	IPGMContext::TYPE_OBJECT:
							((Code	*)object)->mod(member_index,value);	// protected internally.
							break;
						case	IPGMContext::TYPE_GROUP:
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
				}else	if(function[0].asOpcode()==Opcodes::Set){	// args:[iptr-to-cptr value].

					void				*object;
					IPGMContext::ObjectType	object_type;
					int16				member_index;
					uint32				view_oid;
					args.getChild(1).getMember(object,view_oid,object_type,member_index);	// args.getChild(1) is an iptr.

					if(object){

						float32	value=(*args.getChild(2))[0].asFloat();
						switch(object_type){
						case	IPGMContext::TYPE_VIEW:{	// add the target and value to the group's pending operations.

							Group	*g=(Group	*)object;
							g->enter();
							g->pending_operations.push_back(new	Group::Set(view_oid,member_index,value));
							g->leave();
							break;
						}case	IPGMContext::TYPE_OBJECT:
							((Code	*)object)->set(member_index,value);	// protected internally.
							break;
						case	IPGMContext::TYPE_GROUP:
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

				}else	if(function[0].asOpcode()==Opcodes::Prb){		// args:[probe_level,callback_name,msg,set of objects].

					float32	probe_lvl=(*args.getChild(1))[0].asFloat();
					if(probe_lvl<_Mem::Get()->get_probe_level()){

						std::string	callback_name=Utils::GetString(&(*args.getChild(2))[0]);

						Callbacks::Callback	callback=Callbacks::Get(callback_name);
						if(callback){

							std::string	msg=Utils::GetString(&(*args.getChild(3))[0]);
							IPGMContext	_objects=*args.getChild(4);

							uint8	object_count=_objects[0].getAtomCount();
							Code	**objects=NULL;
							if(object_count){

								objects=new	Code	*[object_count];
								for(uint8	i=1;i<=object_count;++i)
									objects[i-1]=(*_objects.getChild(i)).getObject();
							}

							callback(now-Utils::GetTimeReference(),false,msg.c_str(),object_count,objects);
							if(object_count)
								delete[]	objects;
						}
					}
				}else	if(function[0].asOpcode()==Opcodes::Stop){		// no args.

					_Mem::Get()->stop();
				}else{	// unknown function.

					rollback();
					productions.clear();
					return	false;
				}
			}else	if(cmd[0].asOpcode()==Opcodes::Cmd){	// command to an external device, build a cmd object and send it.

				Code	*command=_Mem::Get()->build_object(cmd[0]);
				cmd.copy(command,0);

				_Mem::Get()->eject(command);
				
				Code	*fact=new	Fact(command,now,now,1,1);	// build a fact of the command and inject it in stdin.
				View	*view=new	View(View::SYNC_ONCE,now,1,1,_Mem::Get()->get_stdin(),getView()->get_host(),fact);	//	SYNC_ONCE, sln=1, res=1,
				_Mem::Get()->inject(view);

				if(mk_rdx){

					mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
					(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
				}
			}
		}

		if(mk_rdx){

			mk_rdx->code(mk_rdx_prod_index)=Atom::Set(cmd_count);
			for(uint16	i=1;i<=ntf_grp_count;++i){

				NotificationView	*v=new	NotificationView(getView()->get_host(),getView()->get_host()->get_ntf_grp(i),mk_rdx);
				_Mem::Get()->inject_notification(v,true);
			}
		}

		return	true;
	}

	Code	*InputLessPGMOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(_Mem::Get());

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				// code.
		mk_rdx->add_reference(getObject());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	// inputs.
		mk_rdx->code(extent_index++)=Atom::Set(0);
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	// productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					// psln_thr.

		return	mk_rdx;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PGMOverlay::PGMOverlay(Controller	*c):InputLessPGMOverlay(c){

		is_volatile=c->getObject()->code(IPGM_RES).asBoolean();
		init();
	}

	PGMOverlay::PGMOverlay(PGMOverlay	*original,uint16	last_input_index,uint16	value_commit_index):InputLessPGMOverlay(){

		controller=original->controller;

		input_pattern_indices=original->input_pattern_indices;
		input_pattern_indices.push_back(last_input_index);		// put back the last original's input index.
		for(uint16	i=0;i<original->input_views.size()-1;++i)	// ommit the last original's input view.
			input_views.push_back(original->input_views[i]);

		code_size=original->code_size;
		code=new	r_code::Atom[code_size];
		memcpy(code,original->code,code_size*sizeof(r_code::Atom));	//	copy patched code.

		Atom	*original_code=&getObject()->get_reference(0)->code(0);
		for(uint16	i=0;i<original->patch_indices.size();++i)	//	unpatch code.
			code[original->patch_indices[i]]=original_code[original->patch_indices[i]];

		this->value_commit_index=value_commit_index;
		for(uint16	i=0;i<value_commit_index;++i)	// copy values up to the last commit index.
			values.push_back(original->values[i]);

		is_volatile=original->is_volatile;
		birth_time=original->birth_time;
	}

	inline	PGMOverlay::~PGMOverlay(){
	}

	inline	void	PGMOverlay::init(){

		// init the list of pattern indices.
		uint16	pattern_set_index=code[PGM_INPUTS].asIndex();
		uint16	pattern_count=code[pattern_set_index].getAtomCount();
		for(uint16	i=1;i<=pattern_count;++i)
			input_pattern_indices.push_back(code[pattern_set_index+i].asIndex());

		birth_time=0;
	}

	inline	void	PGMOverlay::reset(){

		InputLessPGMOverlay::reset();
		patch_indices.clear();
		input_views.clear();
		input_pattern_indices.clear();
		init();
	}

	bool	PGMOverlay::is_invalidated(){	
	
		if(is_volatile){

			for(uint32	i=0;i<input_views.size();++i){

				if(input_views[i]->object->is_invalidated())
					return	true;
			}
		}

		return	invalidated==1;
	}

	Code	*PGMOverlay::dereference_in_ptr(Atom	a){

		switch(a.getDescriptor()){
		case	Atom::IN_OBJ_PTR:
			return	getInputObject(a.asInputIndex());
		case	Atom::D_IN_OBJ_PTR:{
			Atom	ptr=code[a.asRelativeIndex()];	// must be either an IN_OBJ_PTR or a D_IN_OBJ_PTR.
			Code	*parent=dereference_in_ptr(ptr);
			return	parent->get_reference(parent->code(ptr.asIndex()).asIndex());
		}default:	// shall never happen.
			return	NULL;
		}
	}

	void	PGMOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index,int16	parent_index){	//	patch recursively : in pgm_code[pgm_code_index] with (D_)IN_OBJ_PTRs until ::.

		uint16	atom_count=code[pgm_code_index].getAtomCount();

		// Replace the head of a structure by a ptr to the input object.
		Atom	head;
		if(parent_index<0)
			head=code[pgm_code_index]=Atom::InObjPointer(input_index,input_code_index);
		else
			head=code[pgm_code_index]=Atom::DInObjPointer(parent_index,input_code_index);
		patch_indices.push_back(pgm_code_index);

		//	Proceed with the structure's members.
		for(uint16	j=1;j<=atom_count;++j){

			uint16	patch_index=pgm_code_index+j;
			switch(code[patch_index].getDescriptor()){
			case	Atom::T_WILDCARD:	// leave as is and stop patching.
				return;
			case	Atom::WILDCARD:
				if(parent_index<0)
					code[patch_index]=Atom::InObjPointer(input_index,input_code_index+j);
				else
					code[patch_index]=Atom::DInObjPointer(parent_index,input_code_index+j);
				patch_indices.push_back(patch_index);
				break;
			case	Atom::I_PTR:{		// sub-structure: go one level deeper in the pattern.
				uint16	indirection=code[patch_index].asIndex();	//	save the indirection before patching.

				if(parent_index<0)
					code[patch_index]=Atom::InObjPointer(input_index,input_code_index+j);
				else
					code[patch_index]=Atom::DInObjPointer(parent_index,input_code_index+j);
				patch_indices.push_back(patch_index);
				switch(dereference_in_ptr(head)->code(j).getDescriptor()){	// caution: the pattern points to sub-structures using iptrs. However, the input object may have a rptr instead of an iptr: we have to disambiguate.
				case	Atom::I_PTR:	// dereference and recurse.
					patch_input_code(indirection,input_index,dereference_in_ptr(head)->code(j).asIndex(),parent_index);
					break;
				case	Atom::R_PTR:	// do not dereference and recurse.
					patch_input_code(indirection,input_index,0,patch_index);
					break;
				default:				// shall never happen.
					break;
				}
				break;
			}default:					// leave as is.
				break;
			}
		}
	}

	Overlay	*PGMOverlay::reduce(r_exec::View	*input){

		uint16	input_index;
		switch(match(input,input_index)){
		case	SUCCESS:
			if(input_pattern_indices.size()==0){	// all patterns matched.

				if(check_guards()	&&	inject_productions()){

					((PGMController	*)controller)->notify_reduction();
					PGMOverlay	*offspring=new	PGMOverlay(this,input_index,value_commit_index);
					invalidate();
					return	offspring;
				}else{

					PGMOverlay	*offspring=new	PGMOverlay(this,input_index,value_commit_index);
					invalidate();
					return	offspring;
				}
			}else{	// create an overlay in a state where the last input is not matched: this overlay will be able to catch other candidates for the input patterns that have already been matched.

				PGMOverlay	*offspring=new	PGMOverlay(this,input_index,value_commit_index);
				commit();
				if(birth_time==0)
					birth_time=Now();
				return	offspring;
			}
		case	FAILURE:	// just rollback: let the overlay match other inputs.
			rollback();
		case	IMPOSSIBLE:
			return	NULL;
		}
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
				rollback();	// to try another pattern on a clean basis.
			case	IMPOSSIBLE:
				break;
			}
		}
		input_views.pop_back();
		return	failed?FAILURE:IMPOSSIBLE;
	}

	inline	PGMOverlay::MatchResult	PGMOverlay::_match(r_exec::View	*input,uint16	pattern_index){

		if(code[pattern_index].asOpcode()==Opcodes::AntiPtn){

			IPGMContext	input_object=IPGMContext::GetContextFromInput(input,this);
			IPGMContext	pattern_skeleton(getObject()->get_reference(0),getView(),code,code[pattern_index+1].asIndex(),this);	// pgm_code[pattern_index] is the first atom of the pattern; pgm_code[pattern_index+1] is an iptr to the skeleton.
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
		}else	if(code[pattern_index].asOpcode()==Opcodes::Ptn){

			IPGMContext	input_object=IPGMContext::GetContextFromInput(input,this);
			IPGMContext	pattern_skeleton(getObject()->get_reference(0),getView(),code,code[pattern_index+1].asIndex(),this);	// pgm_code[pattern_index] is the first atom of the pattern; pgm_code[pattern_index+1] is an iptr to the skeleton.
			if(!pattern_skeleton.match(input_object))
				return	IMPOSSIBLE;
			return	__match(input,pattern_index);
		}
		return	IMPOSSIBLE;
	}

	inline	PGMOverlay::MatchResult	PGMOverlay::__match(r_exec::View	*input,uint16	pattern_index){
//Atom::Trace(pgm_code,getObject()->get_reference(0)->code_size());
//input->object->trace();
		patch_input_code(code[pattern_index+1].asIndex(),input_views.size()-1,0);	// the input has just been pushed on input_views (see match); pgm_code[pattern_index+1].asIndex() is the structure pointed by the pattern's skeleton.
//Atom::Trace(pgm_code,getObject()->get_reference(0)->code_size());
//input->object->trace();
		// match: evaluate the set of guards.
		uint16	guard_set_index=code[pattern_index+2].asIndex();
		if(!evaluate(guard_set_index))
			return	FAILURE;
		return	SUCCESS;
	}

	bool	PGMOverlay::check_guards(){

		uint16	guard_set_index=code[PGM_GUARDS].asIndex();
		uint16	guard_count=code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i){

			if(!evaluate(guard_set_index+i))
				return	false;
		}
		return	true;
	}

	Code	*PGMOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(_Mem::Get());

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				// code.
		mk_rdx->add_reference(getObject());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	// inputs.
		mk_rdx->code(extent_index++)=Atom::Set(input_views.size());
		for(uint16	i=0;i<input_views.size();++i){

			mk_rdx->code(extent_index++)=Atom::RPointer(i+1);
			mk_rdx->add_reference(input_views[i]->object);
		}
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	// productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					// psln_thr.

		return	mk_rdx;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	AntiPGMOverlay::AntiPGMOverlay(Controller	*c):PGMOverlay(c){
	}

	inline	AntiPGMOverlay::AntiPGMOverlay(AntiPGMOverlay	*original,uint16	last_input_index,uint16	value_limit):PGMOverlay(original,last_input_index,value_limit){
	}

	inline	AntiPGMOverlay::~AntiPGMOverlay(){
	}

	Overlay	*AntiPGMOverlay::reduce(r_exec::View	*input){

		uint16	input_index;
		switch(match(input,input_index)){
		case	SUCCESS:
			if(input_pattern_indices.size()==0){	// all patterns matched.

				if(check_guards()){

					((AntiPGMController	*)controller)->restart();
					return	NULL;
				}else{
					rollback();
					return	NULL;
				}
			}else{

				AntiPGMOverlay	*offspring=new	AntiPGMOverlay(this,input_index,value_commit_index);
				commit();
				return	offspring;
			}
		case	FAILURE:	// just rollback: let the overlay match other inputs.
			rollback();
		case	IMPOSSIBLE:
			return	NULL;
		}
	}

}