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

	bool	InputLessPGMOverlay::NeedsAbstraction(Code	*original,SubstitutionData	*substitution_data){

		UNORDERED_MAP<Code	*,std::vector<Substitution> >::const_iterator	s=substitution_data->substitutions.find(original);
		if(s!=substitution_data->substitutions.end())
			return	true;

		for(uint16	i=0;i<original->references_size();++i)
			if(NeedsAbstraction(original->get_reference(i),substitution_data))
				return	true;

		return	false;
	}

	Code	*InputLessPGMOverlay::AbstractObject(Code	*original,SubstitutionData	*substitution_data,bool	head){
		
		UNORDERED_MAP<Code	*,std::vector<Substitution> >::const_iterator	s=substitution_data->substitutions.find(original);
		if(s!=substitution_data->substitutions.end()){

			Code	*abstracted_object=_Mem::Get()->buildObject(original->code(0));
			uint16	i;
			for(i=0;i<original->code_size();++i)	//	copy the code.
				abstracted_object->code(i)=original->code(i);

			for(i=0;i<original->references_size();++i)	//	reset the references.
				abstracted_object->set_reference(i,NULL);
			
			for(uint16	i=0;i<s->second.size();++i){	//	patch atoms/references by atomic variables/variable objects.

				switch(s->second[i].type){
				case	0:	//	numerical variable.
					abstracted_object->code(s->second[i].member_index)=substitution_data->numerical_variables[s->second[i].variable_index];
					break;
				case	1:	//	structural variable.
					abstracted_object->code(abstracted_object->code(s->second[i].member_index).asIndex()+1)=substitution_data->structural_variables[s->second[i].variable_index];
					break;
				case	2:	//	variable object.
					abstracted_object->set_reference(abstracted_object->code(s->second[i].member_index).asIndex(),substitution_data->variable_objects[s->second[i].variable_index]);
					break;
				}
			}

			for(i=0;i<original->references_size();++i)	//	abstract the references left un-abstracted.
				if(abstracted_object->get_reference(i)==NULL)
					abstracted_object->set_reference(i,AbstractObject(original->get_reference(i),substitution_data));

			return	abstracted_object;
		}else	if(head){

			Code	*abstracted_object=_Mem::Get()->buildObject(original->code(0));
			uint16	i;
			for(i=0;i<original->code_size();++i)	//	copy the code.
				abstracted_object->code(i)=original->code(i);

			for(i=0;i<original->references_size();++i)	//	abstract references.
				abstracted_object->set_reference(i,AbstractObject(original->get_reference(i),substitution_data));

			return	abstracted_object;
		}else{
			
			for(uint16	i=0;i<original->references_size();++i){

				if(NeedsAbstraction(original->get_reference(i),substitution_data)){

					Code	*abstracted_object=_Mem::Get()->buildObject(original->code(0));
					uint16	i;
					for(i=0;i<original->code_size();++i)	//	copy the code.
						abstracted_object->code(i)=original->code(i);

					for(i=0;i<original->references_size();++i)	//	abstract references.
						abstracted_object->set_reference(i,AbstractObject(original->get_reference(i),substitution_data));

					return	abstracted_object;
				}
			}
		}

		return	original;
	}

	InputLessPGMOverlay::InputLessPGMOverlay():Overlay(),reduction_mode(RDX_MODE_REGULAR),confidence(1){	//	used for constructing PGMOverlay offsprings.
	}

	InputLessPGMOverlay::InputLessPGMOverlay(Controller	*c):Overlay(c),value_commit_index(0),reduction_mode(RDX_MODE_REGULAR),confidence(1){

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

	void	InputLessPGMOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index,int16	parent_index){
	}

	bool	InputLessPGMOverlay::inject_productions(Controller	*origin){

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
		uint16	cmd_count=0;	//	cmds to the executive (excl. mod/set) and external devices.
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

						object=_Mem::Get()->buildObject(arg1[0]);
//						arg1.trace();
						arg1.copy(object,0);
//						arg1.trace();
//						object->trace();
						if((reduction_mode	&	RDX_MODE_SIMULATION)	||	(reduction_mode	&	RDX_MODE_ASSUMPTION))
							productions.push_back(object);	//	object may be a duplicate: it will be tagged with a mk.sim/mk.asmp.
						else
							productions.push_back(_Mem::Get()->check_existence(object));
					}
					patch_code(index,Atom::ProductionPointer(productions.size()-1));

					++cmd_count;
				}else	if(function[0].asOpcode()!=Opcodes::Mod		&&
							function[0].asOpcode()!=Opcodes::Set	&&
							function[0].asOpcode()!=Opcodes::Prb){

					++cmd_count;
				}
			}else
				++cmd_count;
		}

		Code	*mk_rdx=NULL;
		uint16	ntf_grp_count=getView()->get_host()->get_ntf_grp_count();

		uint16	write_index;
		uint16	extent_index;
		if(ntf_grp_count	&&	cmd_count	&&	(getObject()->code(IPGM_NFR).asBoolean())){	//	the productions are command objects (cmd); only injections/ejections and cmds to external devices are notified.

			mk_rdx=get_mk_rdx(write_index);
			mk_rdx->code(write_index++)=Atom::Set(cmd_count);
			extent_index=write_index+cmd_count;
		}
		
		//	case of abstraction programs: further processing is performed after all substitutions have been executed.
		r_exec::View		*input_view;				//	the view of the object being abstracted.
		RGroup				*r_grp;						//	group where the abstraction takes place.
		SubstitutionData	*substitution_data=NULL;
		Code				*binder;					//	ipgm built according to the substitutions which were used for abstraction.
		uint16				binder_val_index;			//	index of the val_hld i the binder tpl args.
		uint16				binder_reference_index=0;	//	index of the last reference.

		//	all productions have evaluated correctly; now we can execute the commands one by one.
		for(uint16	i=1;i<=production_count;++i){

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

					_Mem::Get()->inject(view);

					if(reduction_mode	&	RDX_MODE_SIMULATION){

						Code	*mk_sim=factory::Object::MkSim(object,((Controller	*)controller)->getObject()->get_reference(0),1);
						View	*mk_view=new	View(view->get_sync(),now,view->get_sln(),view->get_res(),view->get_host(),getView()->get_host(),mk_sim);
						_Mem::Get()->inject(mk_view);
					}

					if(reduction_mode	&	RDX_MODE_ASSUMPTION){

						Code	*mk_asmp=factory::Object::MkAsmp(object,((Controller	*)controller)->getObject()->get_reference(0),confidence,1);
						View	*mk_view=new	View(view->get_sync(),now,view->get_sln(),view->get_res(),view->get_host(),getView()->get_host(),mk_asmp);
						_Mem::Get()->inject(mk_view);
					}

					if(mk_rdx){

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

					_Mem::Get()->eject(view,node[0].getNodeID());

					if(mk_rdx){

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
				}else	if(function[0].asOpcode()==Opcodes::Subst){	//	args:[cptr iptr-to-val_hld].

					void				*object;		//	object holding values to be substitued for, or variables to be bound.
					Context::ObjectType	object_type;
					int16				member_index;	//	points to either a rptr or an iptr to an atomic or structural value.
					uint32				view_oid;
					args.getChild(1).getMember(object,view_oid,object_type,member_index);	//	args.getChild(1) is an iptr to a cptr.
					//args.trace();
					Context	_var=*args.getChild(2);		//	points to a val_hdl: the val member is either (substitution mode) a tolerance, or (binding mode) an atomic variable, a structural variable or a rptr to a variable object.
					//_var.trace();
					Context	_val=*args.getChild(1);		//	dereferenced cptr.
					//_val.trace();
					//	language constraints - no runtime check:
					//	abstraction pgms must specify exactly one input: therefore subst cannot be found in input-less programs or anti-programs.
					RGRPOverlay	*source=(RGRPOverlay	*)((PGMOverlay	*)this)->getSource();
					if(!source){	//	substitution mode.

						if(!substitution_data){
							
							input_view=(r_exec::View*)((PGMOverlay	*)this)->getInputView(0);
							r_grp=((RGroup	*)input_view->get_host());

							substitution_data=new	SubstitutionData();

							Code	*abstractor=getObject();

							//	build the binder as a duplicate of the abstractor, then patch its tpl args.
							binder=_Mem::Get()->buildObject(Atom::InstantiatedProgram(Opcodes::IPgm,IPGM_ARITY));
							for(uint16	i=1;i<abstractor->code_size();++i)
								binder->code(i)=abstractor->code(i);
							for(uint16	i=0;i<abstractor->references_size();++i)
								binder->set_reference(i,abstractor->get_reference(i));
							binder_val_index=binder->code(binder->code(IPGM_ARGS).asIndex()+1).asIndex()+1;
						}
						
						//	find a variable that was abstracted from the same value; if none, a new one is returned: if of class var, it is injected into the r-grp if not already so.
						float32	tolerance=_var[1].asFloat();
						if(_val[0].isFloat()){

							Substitution	s;
							s.member_index=member_index;
							s.type=0;
							s.variable_index=substitution_data->numerical_variables.size();

							Atom	numerical_variable=r_grp->get_numerical_variable(_val[0].asFloat(),tolerance);
							substitution_data->numerical_variables.push_back(numerical_variable);
							substitution_data->substitutions[(Code	*)object].push_back(s);
							binder->code(binder_val_index)=numerical_variable;
						}else	if(object!=_val.getObject()){
							
							Substitution	s;
							s.member_index=member_index;
							s.type=2;
							s.variable_index=substitution_data->variable_objects.size();

							Code	*variable_object=r_grp->get_variable_object(_val.getObject(),tolerance);
							substitution_data->variable_objects.push_back(variable_object);
							substitution_data->substitutions[(Code	*)object].push_back(s);
							binder->code(binder_val_index)=Atom::RPointer(++binder_reference_index);
							binder->set_reference(binder_reference_index,variable_object);
						}else	switch(_val[0].getDescriptor()){	//	structure embedded in object's code.
						case	Atom::OBJECT:
						case	Atom::TIMESTAMP:
						case	Atom::STRING:{

							Substitution	s;
							s.member_index=member_index;
							s.type=1;
							s.variable_index=substitution_data->structural_variables.size();

							Atom	structural_variable=r_grp->get_structural_variable(&((Code	*)object)->code(_val.getIndex()),tolerance);
							substitution_data->structural_variables.push_back(structural_variable);
							substitution_data->substitutions[(Code	*)object].push_back(s);
							binder->code(binder_val_index)=structural_variable;
							break;
						}
						}
						binder_val_index+=VAL_HLD_ARITY;
					}else{	//	binding mode: _var is of class val_hld and holds an existing variable.
						
						if(object!=_val.getObject())
							source->bind(_val.getObject(),_val.getIndex(),_var.getObject(),_var.getIndex()+1);
						else
							source->bind((Code	*)object,_val.getIndex(),_var.getObject(),_var.getIndex()+1);
					}

					if(mk_rdx){

						mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
						(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
					}
				}else	if(function[0].asOpcode()==Opcodes::NewClass){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::DelClass){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::LDC){		// TODO

				}else	if(function[0].asOpcode()==Opcodes::Swap){		// TODO

				}else	if(function[0].asOpcode()==Opcodes::NewDev){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::DelDev){	// TODO

				}else	if(function[0].asOpcode()==Opcodes::Prb){		//	args:[probe_level,callback_name,msg,set of objects].

					float32	probe_lvl=(*args.getChild(1))[0].asFloat();
					if(probe_lvl<_Mem::Get()->get_probe_level()){

						std::string	callback_name=Utils::GetString(&(*args.getChild(2))[0]);

						Callbacks::Callback	callback=Callbacks::Get(callback_name);
						if(callback){

							std::string	msg=Utils::GetString(&(*args.getChild(3))[0]);
							Context	_objects=*args.getChild(4);

							uint8	object_count=_objects[0].getAtomCount();
							Code	**objects=NULL;
							if(object_count){

								objects=new	Code	*[object_count];
								for(uint8	i=1;i<=object_count;++i)
									objects[i-1]=(*_objects.getChild(i)).getObject();
							}
							callback(now-_Mem::Get()->get_starting_time(),false,msg.c_str(),object_count,objects);
							if(object_count)
								delete[]	objects;
						}
					}
				}else	if(function[0].asOpcode()==Opcodes::Suspend){	//	args:[callback_name,msg,set of objects].

					std::string	callback_name=Utils::GetString(&(*args.getChild(1))[0]);
					Callbacks::Callback	callback=Callbacks::Get(callback_name);
					if(callback){

						_Mem::Get()->suspend();

						std::string	msg=Utils::GetString(&(*args.getChild(2))[0]);
						Context	_objects=*args.getChild(3);

						uint8	object_count=_objects[0].getAtomCount();
						Code	**objects=NULL;
						if(object_count){

							objects=new	Code	*[object_count];
							for(uint8	i=1;i<=object_count;++i)
								objects[i-1]=(*_objects.getChild(i)).getObject();
						}
						if(callback(now-_Mem::Get()->get_starting_time(),true,msg.c_str(),object_count,objects))
							_Mem::Get()->resume();
						if(object_count)
							delete[]	objects;
					}
				}else	if(function[0].asOpcode()==Opcodes::Stop){		//	no args.

					_Mem::Get()->stop();
				}else{	//	unknown function.

					rollback();
					productions.clear();
					return	false;
				}
			}else{	//	in case of an external device, create a cmd object and send it.

				Code	*command=_Mem::Get()->buildObject(cmd[0]);
				cmd.copy(command,0);

				_Mem::Get()->eject(command,command->code(CMD_DEVICE).getNodeID());

				if(mk_rdx){

					mk_rdx->code(write_index++)=Atom::IPointer(extent_index);
					(*prods.getChild(i)).copy(mk_rdx,extent_index,extent_index);
				}
			}
		}

		if(substitution_data){

			Code	*original_object=input_view->object;
			Code	*abstracted_object=AbstractObject(original_object,substitution_data,true);

			original_object->acq_views();
			original_object->views.erase(input_view);	//	delete the input view from the original's views.
			if(original_object->views.size()==0)
				original_object->invalidate();
			original_object->rel_views();

			input_view->code(VIEW_SLN)=Atom::Float(0);	//	this prevents the abstraction pgm from reducing the abstracted object.
			input_view->set_object(abstracted_object);
			_Mem::Get()->inject(input_view);

			//	inject a binding ipgm in the r-grp: an instance of the abstraction pgm, passing as template arguments the variables used for substitution.
			View	*binder_view=new	View(true,now,0,-1,r_grp,NULL,binder,1);
			_Mem::Get()->inject(binder_view);

			delete	substitution_data;
		}

		if(mk_rdx){

			for(uint16	i=1;i<=ntf_grp_count;++i){

				NotificationView	*v=new	NotificationView(getView()->get_host(),getView()->get_host()->get_ntf_grp(i),mk_rdx);
				_Mem::Get()->injectNotificationNow(v,true,origin);
			}
		}

		return	true;
	}

	Code	*InputLessPGMOverlay::get_mk_rdx(uint16	&extent_index)	const{

		uint16	write_index=0;
		extent_index=MK_RDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(_Mem::Get());

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

		reduction_mode=RDX_MODE_REGULAR;
		confidence=1;

		birth_time=Now();
	}

	inline	void	PGMOverlay::reset(){

		InputLessPGMOverlay::reset();
		patch_indices.clear();
		input_views.clear();
		input_pattern_indices.clear();
		init();
	}

	Code	*PGMOverlay::dereference_in_ptr(Atom	a){

		switch(a.getDescriptor()){
		case	Atom::IN_OBJ_PTR:
			return	getInputObject(a.asInputIndex());
		case	Atom::D_IN_OBJ_PTR:{
			Atom	ptr=pgm_code[a.asRelativeIndex()];	//	must be either an IN_OBJ_PTR or a D_IN_OBJ_PTR.
			Code	*parent=dereference_in_ptr(ptr);
			return	parent->get_reference(parent->code(ptr.asIndex()).asIndex());
		}default:	//	shall never happen.
			return	NULL;
		}
	}

	void	PGMOverlay::patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index,int16	parent_index){	//	patch recursively : in pgm_code[pgm_code_index] with (D_)IN_OBJ_PTRs until ::.

		uint16	atom_count=pgm_code[pgm_code_index].getAtomCount();

		//	Replace the head of a structure by a ptr to the input object.
		Atom	head;
		if(parent_index<0)
			head=pgm_code[pgm_code_index]=Atom::InObjPointer(input_index,input_code_index);
		else
			head=pgm_code[pgm_code_index]=Atom::DInObjPointer(parent_index,input_code_index);
		patch_indices.push_back(pgm_code_index);

		//	Proceed with the structure's members.
		for(uint16	j=1;j<=atom_count;++j){

			uint16	patch_index=pgm_code_index+j;
			switch(pgm_code[patch_index].getDescriptor()){
			case	Atom::T_WILDCARD:	//	leave as is and stop patching.
				return;
			case	Atom::WILDCARD:
				if(parent_index<0)
					pgm_code[patch_index]=Atom::InObjPointer(input_index,input_code_index+j);
				else
					pgm_code[patch_index]=Atom::DInObjPointer(parent_index,input_code_index+j);
				patch_indices.push_back(patch_index);
				break;
			case	Atom::I_PTR:{		//	sub-structure: go one level deeper in the pattern.
				uint16	indirection=pgm_code[patch_index].asIndex();	//	save the indirection before patching.

				if(parent_index<0)
					pgm_code[patch_index]=Atom::InObjPointer(input_index,input_code_index+j);
				else
					pgm_code[patch_index]=Atom::DInObjPointer(parent_index,input_code_index+j);
				patch_indices.push_back(patch_index);
				switch(dereference_in_ptr(head)->code(j).getDescriptor()){	//	caution: the pattern points to sub-structures using iptrs. However, the input object may have a rptr instead of an iptr: we have to disambiguate.
				case	Atom::I_PTR:	//	dereference and recurse.
					patch_input_code(indirection,input_index,dereference_in_ptr(head)->code(j).asIndex(),parent_index);
					break;
				case	Atom::R_PTR:	//	do not dereference and recurse.
					patch_input_code(indirection,input_index,0,patch_index);
					break;
				default:				//	shall never happen.
					break;
				}
				break;
			}default:					//	leave as is.
				break;
			}
		}
	}

	void	PGMOverlay::_reduce(r_exec::View	*input){

		if(alive){

			uint8	old_reduction_mode=reduction_mode;
			float32	old_confidence=confidence;
			uint16	input_index;
			bool	sim=false;
			Code	*mk_asmp=input->object->get_asmp();
			if(input->object->get_hyp()	||	input->object->get_sim())
				sim=true;
			switch(match(input,input_index)){
			case	SUCCESS:
				if(sim)
					reduction_mode|=RDX_MODE_SIMULATION;
				if(mk_asmp){

					reduction_mode|=RDX_MODE_ASSUMPTION;
					float32	cfd=mk_asmp->code(MK_ASMP_CFD).asFloat();
					if(confidence>cfd)
						confidence=cfd;
				}
				if(input_pattern_indices.size()==0){	//	all patterns matched.

					if(check_timings()	&&	check_guards()	&&	inject_productions(NULL)){

						((PGMController	*)controller)->notify_reduction();
						controller->remove(this);
						return;
					}else{
						reduction_mode=old_reduction_mode;
						confidence=old_confidence;
						rollback();
						return;
					}
				}else{	//	create an overlay in a state where the last input is not matched: this overlay will be able to catch other candidates for the input patterns that have already been matched.

					PGMOverlay	*offspring=new	PGMOverlay(this,input_index,value_commit_index);
					offspring->reduction_mode=old_reduction_mode;
					offspring->confidence=old_confidence;
					controller->add(offspring);
					commit();
					return;
				}
			case	FAILURE:	//	just rollback: let the overlay match other inputs.
				rollback();
				return;
			}
		}
	}

	void	PGMOverlay::reduce(r_exec::View	*input){

		reductionCS.enter();
		this->source=NULL;
		_reduce(input);
		reductionCS.leave();
	}

	void	PGMOverlay::reduce(r_exec::View	*input,Overlay	*source){

		reductionCS.enter();
		this->source=source;
		_reduce(input);
		this->source=NULL;
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

		if(pgm_code[pattern_index].asOpcode()==Opcodes::AntiPtn){

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
		}else	if(pgm_code[pattern_index].asOpcode()==Opcodes::Ptn){

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

		Code	*mk_rdx=new	r_exec::LObject(_Mem::Get());

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

	void	AntiPGMOverlay::reduce(r_exec::View	*input){

		reductionCS.enter();

		if(alive){

			uint16	input_index;
			switch(match(input,input_index)){
			case	SUCCESS:
				if(input_pattern_indices.size()==0){	//	all patterns matched.

					if(check_timings()	&&	check_guards()){

						((AntiPGMController	*)controller)->restart(this);
						break;
					}else{
						rollback();
						break;
					}
				}else{

					AntiPGMOverlay	*offspring=new	AntiPGMOverlay(this,input_index,value_commit_index);
					controller->add(offspring);
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

		Code	*mk_rdx=new	r_exec::LObject(_Mem::Get());

		mk_rdx->code(write_index++)=Atom::Marker(Opcodes::MkAntiRdx,MK_ANTI_RDX_ARITY);
		mk_rdx->code(write_index++)=Atom::RPointer(0);				//	code.
		mk_rdx->add_reference(getObject());
		mk_rdx->code(write_index++)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(write_index++)=Atom::Float(1);					//	psln_thr.

		return	mk_rdx;
	}

}