//	context.cpp
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

#include	"context.h"
#include	"pgm_overlay.h"
#include	"operator.h"
#include	"opcodes.h"
#include	"mem.h"


using	namespace	r_code;

namespace	r_exec{

	bool	IPGMContext::operator	==(const	IPGMContext	&c)	const{
//c.trace();
//this->trace();
		IPGMContext	lhs=**this;
		IPGMContext	rhs=*c;

		if(lhs.data==REFERENCE	&&	lhs.index==0	&&
			rhs.data==REFERENCE	&&	rhs.index==0)	//	both point to an object's head, not one of its members.
			return	lhs.object==rhs.object;

		//	both contexts point to an atom which is not a pointer.
		if(lhs[0]!=rhs[0])
			return	false;

		if(lhs[0].isStructural()){	//	both are structural.

			uint16	atom_count=lhs.getChildrenCount();
			for(uint16	i=1;i<=atom_count;++i)
				if(*lhs.getChild(i)!=*rhs.getChild(i))
					return	false;
			return	true;
		}
		return	true;
	}

	inline	bool	IPGMContext::operator	!=(const	IPGMContext	&c)	const{

		return	!(*this==c);
	}

	bool	IPGMContext::match(const	IPGMContext	&input)	const{

		if(data==REFERENCE	&&	index==0	&&
			input.data==REFERENCE	&&	input.index==0){	//	both point to an object's head, not one of its members.

			if(object==input.object)
				return	true;
			return	false;
		}

		if(code[index].isStructural()){

			uint16	atom_count=getChildrenCount();
			if(input.getChildrenCount()!=atom_count)
				return	false;

			if(((*this)[0].atom	&	0xFFFFFFF8)!=(input[0].atom	&	0xFFFFFFF8))	//	masking a possible tolerance embedded in timestamps; otherwise, the last 3 bits encode an arity.
				return	false;

			for(uint16	i=1;i<=atom_count;++i){

				IPGMContext	pc=*getChild(i);
				IPGMContext	ic=*input.getChild(i);
				if(!pc.match(ic))
					return	false;
			}
			return	true;
		}

		switch((*this)[0].getDescriptor()){
		case	Atom::WILDCARD:
		case	Atom::T_WILDCARD:
			return	true;
		case	Atom::IPGM_PTR:
			return	(**this).match(input);
		default:
			return	(*this)[0]==input[0];
		}
	}

	void	IPGMContext::dereference_once(){

		switch((*this)[0].getDescriptor()){
		case	Atom::I_PTR:
			index=(*this)[0].asIndex();
			break;
		default:
			break;
		}
	}

	IPGMContext	IPGMContext::operator	*()	const{

		switch((*this)[0].getDescriptor()){
		case	Atom::VL_PTR:{	//	evaluate the code if necessary.
			//	TODO: OPTIMIZATION: if not in a cptr and if this eventually points to an r-ptr or in-obj-ptr,
			//	patch the code at this->index, i.e. replace the vl ptr by the in-obj-ptr or rptr.
			Atom	a=code[(*this)[0].asIndex()];
			uint16	structure_index;
			if(a.getDescriptor()==Atom::I_PTR)	//	dereference once.
				a=code[structure_index=a.asIndex()];
			if(a.isStructural()){	//	the target location is not evaluated yet.

				IPGMContext	s(object,view,code,structure_index,(InputLessPGMOverlay	*)overlay,data);
				uint16	unused_index;
				if(s.evaluate_no_dereference(unused_index))
					return	s;	//	patched code: atom at VL_PTR's index changed to VALUE_PTR or 32 bits result.
				else	//	evaluation failed, return undefined context.
					return	IPGMContext();
			}else
				return	*IPGMContext(object,view,code,(*this)[0].asIndex(),(InputLessPGMOverlay	*)overlay,data);
		}case	Atom::I_PTR:
			return	*IPGMContext(object,view,code,(*this)[0].asIndex(),(InputLessPGMOverlay	*)overlay,data);
		case	Atom::R_PTR:{
			Code	*o=object->get_reference((*this)[0].asIndex());
			return	IPGMContext(o,NULL,&o->code(0),0,NULL,REFERENCE);
		}case	Atom::C_PTR:{
			
			IPGMContext	c=*getChild(1);
			for(uint16	i=2;i<=getChildrenCount();++i){

				switch((*this)[i].getDescriptor()){
				case	Atom::VIEW:	//	accessible only for this and input objects.
					if(c.view)
						c=IPGMContext(c.getObject(),c.view,&c.view->code(0),0,NULL,VIEW);
					else
						return	IPGMContext();
					break;
				case	Atom::MKS:
					return	IPGMContext(c.getObject(),MKS);
					break;
				case	Atom::VWS:
					return	IPGMContext(c.getObject(),VWS);
					break;
				default:
					c=*c.getChild((*this)[i].asIndex());
				}
			}
			return	c;
		}
		case	Atom::THIS:	//	refers to the ipgm; the pgm view is not available.
			return	IPGMContext(overlay->getObject(),overlay->getView(),&overlay->getObject()->code(0),0,(InputLessPGMOverlay	*)overlay,REFERENCE);
		case	Atom::VIEW:	//	never a reference, always in a cptr.
			if(overlay	&&	object==overlay->getObject())
				return	IPGMContext(object,view,&view->code(0),0,NULL,VIEW);
			return	*this;
		case	Atom::MKS:
			return	IPGMContext(object,MKS);
		case	Atom::VWS:
			return	IPGMContext(object,VWS);
		case	Atom::VALUE_PTR:
			return	IPGMContext(object,view,&overlay->values[0],(*this)[0].asIndex(),(InputLessPGMOverlay	*)overlay,VALUE_ARRAY);
		case	Atom::IPGM_PTR:
			return	*IPGMContext(overlay->getObject(),(*this)[0].asIndex());
		case	Atom::IN_OBJ_PTR:{
			Code	*input_object=((PGMOverlay	*)overlay)->getInputObject((*this)[0].asInputIndex());
			View	*input_view=(r_exec::View*)((PGMOverlay	*)overlay)->getInputView((*this)[0].asInputIndex());
			return	*IPGMContext(input_object,input_view,&input_object->code(0),(*this)[0].asIndex(),NULL,REFERENCE);
		}case	Atom::D_IN_OBJ_PTR:{
			IPGMContext	parent=*IPGMContext(object,view,code,(*this)[0].asRelativeIndex(),(InputLessPGMOverlay	*)overlay,data);
			Code	*parent_object=parent.object;
			return	*IPGMContext(parent_object,NULL,&parent_object->code(0),(*this)[0].asIndex(),NULL,REFERENCE);
		}case	Atom::PROD_PTR:
			return	IPGMContext(((InputLessPGMOverlay	*)overlay)->productions[(*this)[0].asIndex()],0);
		default:
			return	*this;
		}
	}

	bool	IPGMContext::evaluate_no_dereference(uint16	&result_index)	const{

		switch(data){
		case	REFERENCE:
		case	VALUE_ARRAY:
			return	true;
		case	UNDEFINED:
			return	false;
		case	VIEW:
		case	MKS:
		case	VWS:
			result_index=index;
			return	true;
		}
		switch(code[index].getDescriptor()){
		case	Atom::OPERATOR:{

			Operator	op=Operator::Get((*this)[0].asOpcode());
			if(op.is_syn())
				return	true;
			if(!op.is_red()){	//	red will prevent the evaluation of its productions before reducting its input.

				uint16	atom_count=getChildrenCount();
				for(uint16	i=1;i<=atom_count;++i){

					uint16	unused_result_index;
					if(!(*getChild(i)).evaluate_no_dereference(unused_result_index))
						return	false;
				}
			}
			IPGMContext	*__c=new	IPGMContext(*this);
			Context	_c(__c);
			return	op(_c,result_index);
		}case	Atom::OBJECT:	//	incl. cmd.
			if((*this)[0].asOpcode()==Opcodes::Ptn	||	(*this)[0].asOpcode()==Opcodes::AntiPtn){	//	skip patterns.

				result_index=index;
				return	true;
			}
		case	Atom::MARKER:
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::INSTANTIATED_CPP_PROGRAM:
		case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
		case	Atom::INSTANTIATED_ANTI_PROGRAM:
		case	Atom::COMPOSITE_STATE:
		case	Atom::MODEL:
		case	Atom::GROUP:
		case	Atom::SET:
		case	Atom::S_SET:{

			uint16	atom_count=getChildrenCount();
			for(uint16	i=1;i<=atom_count;++i){

				uint16	unused_result_index;
				if(!(*getChild(i)).evaluate_no_dereference(unused_result_index))
					return	false;
			}
			result_index=index;
			return	true;
		}default:
			result_index=index;
			return	true;
		}
	}

	inline	bool	IPGMContext::evaluate(uint16	&result_index)	const{

		if(data==REFERENCE	||	data==VALUE_ARRAY)
			return	true;

		IPGMContext	c=**this;
		return	c.evaluate_no_dereference(result_index);
	}

	void	IPGMContext::copy_to_value_array(uint16	&position){

		position=overlay->values.size();
		uint16	extent_index;
		if(code[index].isStructural())
			copy_structure_to_value_array(false,overlay->values.size(),extent_index,true);
		else
			copy_member_to_value_array(0,false,overlay->values.size(),extent_index,true);
	}

	void	IPGMContext::copy_structure_to_value_array(bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr){	//	prefix: by itpr or not.

		if(code[index].getDescriptor()==Atom::OPERATOR	&&	Operator::Get(code[index].asOpcode()).is_syn())
			copy_member_to_value_array(1,prefix,write_index,extent_index,dereference_cptr);
		else{

			uint16	atom_count=getChildrenCount();
			overlay->values[write_index++]=code[index];
			extent_index=write_index+atom_count;
			switch(code[index].getDescriptor()){
			case	Atom::TIMESTAMP:
				for(uint16	i=1;i<=atom_count;++i)
					overlay->values[write_index++]=code[index+i];
				break;
			case	Atom::C_PTR:
				if(!dereference_cptr){

					copy_member_to_value_array(1,prefix,write_index++,extent_index,false);
					for(uint16	i=2;i<=atom_count;++i)
						overlay->values[write_index++]=code[index+i];
					//Atom::Trace(&overlay->values[0],overlay->values.size());
					break;
				}	//	else, dereference the c_ptr.
			default:
				if(is_cmd_with_cptr()){

					for(uint16	i=1;i<=atom_count;++i)
						copy_member_to_value_array(i,prefix,write_index++,extent_index,i!=2);
				}else{

					for(uint16	i=1;i<=atom_count;++i)
						copy_member_to_value_array(i,prefix,write_index++,extent_index,!(!dereference_cptr	&&	i==1));
				}
			}
		}
	}

	void	IPGMContext::copy_member_to_value_array(uint16	child_index,bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr){

		uint16	_index=index+child_index;
		Atom	head;
dereference:
		head=code[_index];
		switch(head.getDescriptor()){	//	dereference until either we reach some non-pointer or a reference or a value pointer.
		case	Atom::I_PTR:
		case	Atom::VL_PTR:
			_index=head.asIndex();
			goto	dereference;
		case	Atom::VALUE_PTR:
			overlay->values[write_index]=Atom::IPointer(head.asIndex());
			return;
		case	Atom::PROD_PTR:
		case	Atom::IN_OBJ_PTR:
		case	Atom::D_IN_OBJ_PTR:
		case	Atom::R_PTR:
			overlay->values[write_index]=head;
			return;
		case	Atom::C_PTR:
			if(dereference_cptr){

				uint16	saved_index=index;
				index=_index;
				IPGMContext	cptr=**this;
				overlay->values[write_index]=cptr[0];
				index=saved_index;
				return;
			}
		}

		if(head.isStructural()){

			uint16	saved_index=index;
			index=_index;
			if(prefix){

				overlay->values[write_index]=Atom::IPointer(extent_index);
				copy_structure_to_value_array(true,extent_index,extent_index,dereference_cptr);
			}else
				copy_structure_to_value_array(true,write_index,extent_index,dereference_cptr);
			index=saved_index;
		}else
			overlay->values[write_index]=head;
	}

	void	IPGMContext::getMember(void	*&object,uint32	&view_oid,ObjectType	&object_type,int16	&member_index)	const{

		if((*this)[0].getDescriptor()!=Atom::I_PTR){	//	ill-formed mod/set expression.

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		IPGMContext	cptr=IPGMContext(this->object,view,code,(*this)[0].asIndex(),(InputLessPGMOverlay	*)overlay,data);	//	dereference manually (1 offset) to retain the cptr as is.

		if(cptr[0].getDescriptor()!=Atom::C_PTR){	//	ill-formed mod/set expression.

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		IPGMContext	c=*cptr.getChild(1);	//	this, vl_ptr, value_ptr or rptr.

		uint16	atom_count=cptr.getChildrenCount();
		for(uint16	i=2;i<atom_count;++i){	//	stop before the last iptr.

			if(cptr[i].getDescriptor()==Atom::VIEW)
				c=IPGMContext(c.getObject(),c.view,&c.view->code(0),0,NULL,VIEW);
			else
				c=*c.getChild(cptr[i].asIndex());
		}
		
		//	at this point, c is an iptr dereferenced to an object or a view; the next iptr holds the member index.
		//	c is pointing at the first atom of an object or a view.
		switch(c[0].getDescriptor()){
		case	Atom::OBJECT:
		case	Atom::MARKER:
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::INSTANTIATED_CPP_PROGRAM:
		case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
		case	Atom::INSTANTIATED_ANTI_PROGRAM:
		case	Atom::COMPOSITE_STATE:
		case	Atom::MODEL:
			object_type=TYPE_OBJECT;
			object=c.getObject();
			break;
		case	Atom::GROUP:
		case	Atom::SET:		//	dynamically generated views can be sets.
		case	Atom::S_SET:	//	views are always copied; set the object to the view's group on which to perform an operation for the view's oid.
			if(c.data==VALUE_ARRAY)
				object=(Group	*)c[VIEW_CODE_MAX_SIZE].atom;	//	first reference of grp in a view stored in athe value array.
			else
				object=c.view->get_host();
			view_oid=c[VIEW_OID].atom;	//	oid is hidden at the end of the view code; stored directly as a uint32.
			object_type=TYPE_VIEW;
			break;
		default:	//	atomic value or ill-formed mod/set expression.
			object=NULL;
			object_type=TYPE_UNDEFINED;
			return;
		}

		//	get the index held by the last iptr.
		member_index=cptr[atom_count].asIndex();
	}

	bool	IPGMContext::is_cmd_with_cptr()	const{

		if((*this)[0].getDescriptor()!=Atom::OBJECT)
			return	false;
		if((*this)[0].asOpcode()!=Opcodes::ICmd)
			return	false;
		return	(*this)[1].asOpcode()==Opcodes::Mod	||
				(*this)[1].asOpcode()==Opcodes::Set;
	}

	uint16	IPGMContext::addProduction(Code	*object,bool	check_for_existence)	const{	//	called by operators (ins and red).
		
		((InputLessPGMOverlay	*)overlay)->productions.push_back(_Mem::Get()->check_existence(object));
		return	((InputLessPGMOverlay	*)overlay)->productions.size()-1;
	}

	//	Utilities for executive-dependent operators	////////////////////////////////////////////////////////////////////////////////

	bool	match(const	IPGMContext	&input,const	IPGMContext	&pattern){	//	in red, patterns like (ptn object: [guards]) are allowed.

		//	patch the pattern with a ptr to the input.
		if(input.is_reference()){

			uint16	ptr=pattern.addProduction(input.getObject(),false);	//	the object obviously is not new.
			pattern.patch_code(pattern.getIndex()+1,Atom::ProductionPointer(ptr));
		}else
			pattern.patch_code(pattern.getIndex()+1,Atom::IPointer(input.getIndex()));

		//	evaluate the set of guards.
		IPGMContext	guard_set=*pattern.getChild(2);
		uint16	guard_count=guard_set.getChildrenCount();
		uint16	unused_index;
		for(uint16	i=1;i<=guard_count;++i){

			if(!(*guard_set.getChild(i)).evaluate_no_dereference(unused_index))	//	WARNING: no check for duplicates.
				return	false;
		}

		return	true;
	}

	bool	match(const	IPGMContext	&input,const	IPGMContext	&pattern,const	IPGMContext	&productions,std::vector<uint16>	&production_indices){

		IPGMContext	&skeleton=IPGMContext();
		uint16	last_patch_index;
		if(pattern[0].asOpcode()==Opcodes::Ptn){
			
			skeleton=*pattern.getChild(1);
			if(!skeleton.match(input))
				return	false;
			last_patch_index=pattern.get_last_patch_index();
			if(!match(input,pattern))
				return	false;

			goto	build_productions;
		}

		if(pattern[0].asOpcode()==Opcodes::AntiPtn){
			
			skeleton=*pattern.getChild(1);
			if(skeleton.match(input))
				return	false;
			last_patch_index=pattern.get_last_patch_index();
			if(match(input,pattern))
				return	false;

			goto	build_productions;
		}

		return	false;

build_productions:
		//	compute all productions for this input.
		uint16	production_count=productions.getChildrenCount();
		uint16	unused_index;
		uint16	production_index;
		for(uint16	i=1;i<=production_count;++i){

			IPGMContext	prod=productions.getChild(i);
//prod.trace();
			prod.evaluate(unused_index);
			prod.copy_to_value_array(production_index);
			production_indices.push_back(production_index);
		}

		pattern.unpatch_code(last_patch_index);
		return	true;
	}

	void	reduce(const	IPGMContext	&context,const	IPGMContext	&input_set,const	IPGMContext	&section,std::vector<uint16>	&input_indices,std::vector<uint16>	&production_indices){

		IPGMContext	pattern=*section.getChild(1);
		if(pattern[0].asOpcode()!=Opcodes::Ptn	&&	pattern[0].asOpcode()!=Opcodes::AntiPtn)
			return;

		IPGMContext	productions=*section.getChild(2);
		if(productions[0].getDescriptor()!=Atom::SET)
			return;

		uint16	production_count=productions.getChildrenCount();
		if(!production_count)
			return;

		std::vector<uint16>::iterator	i;
		for(i=input_indices.begin();i!=input_indices.end();){	//	to be successful, at least one input must match the pattern.

			IPGMContext	c=*input_set.getChild(*i);
			if(c.is_undefined()){

				i=input_indices.erase(i);
				continue;
			}

			bool	failure=false;
			if(!match(c,pattern,productions,production_indices)){

				failure=true;
				break;
			}

			if(failure)
				++i;
			else	//	pattern matched: remove the input from the todo list.
				i=input_indices.erase(i);
		}
	}

	bool	IPGMContext::Red(const	IPGMContext	&context,uint16	&index){
//context.trace();
		uint16	unused_result_index;
		IPGMContext	input_set=*context.getChild(1);
		if(!input_set.evaluate_no_dereference(unused_result_index))
			return	false;

		//	a section is a set of one pattern and a set of productions.
		IPGMContext	positive_section=*context.getChild(2);
		if(!(*positive_section.getChild(1)).evaluate_no_dereference(unused_result_index))	//	evaluate the pattern only.
			return	false;

		IPGMContext	negative_section=*context.getChild(3);
		if(!(*negative_section.getChild(1)).evaluate_no_dereference(unused_result_index))	//	evaluate the pattern only.
			return	false;

		std::vector<uint16>	input_indices;		//	todo list of inputs to match.
		for(uint16	i=1;i<=input_set.getChildrenCount();++i)
			input_indices.push_back(i);

		std::vector<uint16>	production_indices;	//	list of productions built upon successful matches.

		if(input_set[0].getDescriptor()!=Atom::SET	&&
			input_set[0].getDescriptor()!=Atom::S_SET	&&
			positive_section[0].getDescriptor()!=Atom::SET	&&
			negative_section[0].getDescriptor()!=Atom::SET)
			goto	failure;

		uint16	input_count=input_set.getChildrenCount();
		if(!input_count)
			goto	failure;

		reduce(context,input_set,positive_section,input_indices,production_indices);	//	input_indices now filled only with the inputs that did not match the positive pattern.
		reduce(context,input_set,negative_section,input_indices,production_indices);	//	input_indices now filled only with the inputs that did not match the positive nor the negative pattern.
		if(production_indices.size()){

			//	build the set of all productions in the value array.
			index=context.setCompoundResultHead(Atom::Set(production_indices.size()));
			for(uint16	i=0;i<production_indices.size();++i)	//	fill the set with iptrs to productions: the latter are copied in the value array.
				context.addCompoundResultPart(Atom::IPointer(production_indices[i]));
			//(*context).trace();
			return	true;
		}
failure:
		index=context.setCompoundResultHead(Atom::Set(0));
		return	false;
	}

	bool	IPGMContext::Ins(const	IPGMContext	&context,uint16	&index){

		IPGMContext	object=*context.getChild(1);
		IPGMContext	args=*context.getChild(2);
		IPGMContext	run=*context.getChild(3);
		IPGMContext	tsc=*context.getChild(4);
		IPGMContext	nfr=*context.getChild(5);

		Code	*_object=object.getObject();
		if(_object->code(0).asOpcode()!=Opcodes::Pgm	&&	_object->code(0).asOpcode()!=Opcodes::AntiPgm){

			context.setAtomicResult(Atom::Nil());
			return	false;
		}

		if(_object	&&	args[0].getDescriptor()==Atom::SET){

			uint16	pattern_set_index=_object->code(PGM_TPL_ARGS).asIndex();	//	same index for goals.
			uint16	arg_count=args[0].getAtomCount();
			if(_object->code(pattern_set_index).getAtomCount()!=arg_count){

				context.setAtomicResult(Atom::Nil());
				return	false;
			}
			
			//	match args with the tpl patterns in _object.
			IPGMContext	pattern_set(_object,pattern_set_index);
			for(uint16	i=1;i<=arg_count;++i){

				IPGMContext	arg=*args.getChild(i);
				IPGMContext	skel=*(*pattern_set.getChild(i)).getChild(1);
				if(!skel.match(arg)){

					context.setAtomicResult(Atom::Nil());
					return	false;
				}
			}

			//	create an ipgm in the production array.
			Code	*ipgm=context.build_object(_object->code(0));

			ipgm->code(0)=Atom::InstantiatedProgram(Opcodes::IPgm,IPGM_ARITY);
			ipgm->code(IPGM_PGM)=Atom::RPointer(0);				//	points to the pgm object.

			uint16	extent_index=0;
			ipgm->code(IPGM_ARGS)=Atom::IPointer(IPGM_ARITY+1);	//	points to the arg set.
			args.copy(ipgm,IPGM_ARITY+1,extent_index);			//	writes the args after psln_thr.
			
			ipgm->code(IPGM_RUN)=run[0];
			ipgm->code(IPGM_TSC)=Atom::IPointer(extent_index);	//	points to the tsc.

			ipgm->code(extent_index++)=tsc[0];					//	writes the tsc after the args.
			ipgm->code(extent_index++)=tsc[1];
			ipgm->code(extent_index++)=tsc[2];

			ipgm->code(IPGM_NFR)=nfr[0];						//	nfr.
			ipgm->code(IPGM_ARITY)=Atom::Float(1);				//	psln_thr.

			ipgm->set_reference(0,_object);
			
			context.setAtomicResult(Atom::ProductionPointer(context.addProduction(ipgm,true)));	// object may be new: we don't know at this point, therefore check=true.
			return	true;
		}
		
		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	bool	IPGMContext::Fvw(const	IPGMContext	&context,uint16	&index){

		IPGMContext	object=*context.getChild(1);
		IPGMContext	group=*context.getChild(2);

		Code	*_object=object.getObject();
		Group	*_group=(Group	*)group.getObject();
		if(!_object	||	!_group){

			context.setAtomicResult(Atom::Nil());
			return	false;
		}

		View	*v=(View	*)_object->get_view(_group,true);	// returns (a copy of: deprecated) of the view, if any.
		if(v){	// copy the view in the value array: code on VIEW_CODE_MAX_SIZE followed by 2 atoms holding raw pointers to grp and org.

			index=context.setCompoundResultHead(v->code(0));
			for(uint16	i=1;i<VIEW_CODE_MAX_SIZE;++i)
				context.addCompoundResultPart(v->code(i));
			context.addCompoundResultPart(Atom((uint32)v->references[0]));
			context.addCompoundResultPart(Atom((uint32)v->references[1]));
			delete	v;
			return	true;
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}
}