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

	bool	Context::operator	==(const	Context	&c)	const{
//c.trace();
//this->trace();
		Context	lhs=**this;
		Context	rhs=*c;

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

	bool	Context::match(const	Context	&input)	const{
//input.trace();
//this->trace();
		if(data==REFERENCE	&&	index==0	&&
			input.data==REFERENCE	&&	input.index==0)	//	both point to an object's head, not one of its members.
			return	object==input.object;

		if(code[index].isStructural()){

			uint16	atom_count=getChildrenCount();
			if(input.getChildrenCount()!=atom_count)
				return	false;

			if((*this)[0].atom!=input[0].atom)
				return	false;

			for(uint16	i=1;i<=atom_count;++i){

				Context	pc=*getChild(i);
				Context	ic=*input.getChild(i);
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

	inline	bool	Context::operator	!=(const	Context	&c)	const{

		return	!(*this==c);
	}

	void	Context::dereference_once(){

		switch((*this)[0].getDescriptor()){
		case	Atom::I_PTR:
			index=(*this)[0].asIndex();
			break;
		default:
			break;
		}
	}

	Context	Context::operator	*()	const{

		switch((*this)[0].getDescriptor()){
		case	Atom::VL_PTR:{	//	evaluate the code if necessary.
			//	TODO: OPTIMIZATION: if not in a cptr and if this eventually points to an r-ptr or in-obj-ptr,
			//	patch the code at this->index, i.e. replace the vl ptr by the in-obj-ptr or rptr.
			Atom	a=code[(*this)[0].asIndex()];
			uint16	structure_index;
			if(a.getDescriptor()==Atom::I_PTR)	//	dereference once.
				a=code[structure_index=a.asIndex()];
			if(a.isStructural()){	//	the target location is not evaluated yet.

				Context	s(object,view,code,structure_index,overlay,data);
				uint16	unused_index;
				if(s.evaluate_no_dereference(unused_index))
					return	s;	//	patched code: atom at VL_PTR's index changed to VALUE_PTR or 32 bits result.
				else	//	evaluation failed, return undefined context.
					return	Context();
			}else
				return	*Context(object,view,code,(*this)[0].asIndex(),overlay,data);
		}case	Atom::I_PTR:
			return	*Context(object,view,code,(*this)[0].asIndex(),overlay,data);
		case	Atom::R_PTR:{

			Code	*o=object->get_reference((*this)[0].asIndex());
			return	Context(o,NULL,&o->code(0),0,NULL,REFERENCE);
		}case	Atom::C_PTR:{
			
			Context	c=*getChild(1);
			for(uint16	i=2;i<=getChildrenCount();++i){

				switch((*this)[i].getDescriptor()){
				case	Atom::VIEW:	//	accessible only for this and input objects.
					if(c.view)
						c=Context(c.getObject(),c.view,&c.view->code(0),0,NULL,VIEW);
					else
						return	Context();
					break;
				case	Atom::MKS:
					return	Context(c.getObject(),MKS);
					break;
				case	Atom::VWS:
					return	Context(c.getObject(),VWS);
					break;
				default:
					c=*c.getChild((*this)[i].asIndex());
				}
			}
			return	c;
		}
		case	Atom::THIS:	//	refers to the ipgm; the pgm view is not available.
			return	Context(overlay->getObject(),overlay->getView(),&overlay->getObject()->code(0),0,overlay,REFERENCE);
		case	Atom::VIEW:	//	never a reference, always in a cptr.
			if(overlay	&&	object==overlay->getObject())
				return	Context(object,view,&view->code(0),0,NULL,VIEW);
			return	*this;
		case	Atom::MKS:
			return	Context(object,MKS);
		case	Atom::VWS:
			return	Context(object,VWS);
		case	Atom::VALUE_PTR:
			return	Context(object,view,&overlay->values[0],(*this)[0].asIndex(),overlay,VALUE_ARRAY);
		case	Atom::IPGM_PTR:
			return	*Context(overlay->getObject(),(*this)[0].asIndex());
		case	Atom::IN_OBJ_PTR:{
			Code	*input_object=((PGMOverlay	*)overlay)->getInputObject((*this)[0].asInputIndex());
			View	*input_view=(r_exec::View*)((PGMOverlay	*)overlay)->getInputView((*this)[0].asInputIndex());
			return	*Context(input_object,input_view,&input_object->code(0),(*this)[0].asIndex(),NULL,REFERENCE);
		}case	Atom::PROD_PTR:
			return	Context(overlay->productions[(*this)[0].asIndex()],0);
		default:
			return	*this;
		}
	}

	bool	Context::evaluate_no_dereference(uint16	&result_index)	const{

		if(data==REFERENCE	||	data==VALUE_ARRAY)
			return	true;

		switch(data){
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
			return	op(*this,result_index);
		}case	Atom::OBJECT:	//	incl. cmd.
			if((*this)[0].asOpcode()==Opcodes::Ptn	||	(*this)[0].asOpcode()==Opcodes::AntiPtn){	//	skip patterns.

				result_index=index;
				return	true;
			}
		case	Atom::MODEL:
		case	Atom::MARKER:
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::INSTANTIATED_CPP_PROGRAM:
		case	Atom::GROUP:
		case	Atom::REDUCTION_GROUP:
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

	inline	bool	Context::evaluate(uint16	&result_index)	const{

		if(data==REFERENCE	||	data==VALUE_ARRAY)
			return	true;

		Context	c=**this;
		return	c.evaluate_no_dereference(result_index);
	}

	void	Context::copy_to_value_array(uint16	&position){

		position=overlay->values.size();
		uint16	extent_index;
		if(code[index].isStructural())
			copy_structure_to_value_array(false,overlay->values.size(),extent_index,true);
		else
			copy_member_to_value_array(0,false,overlay->values.size(),extent_index,true);
	}

	void	Context::copy_structure_to_value_array(bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr){	//	prefix: by itpr or not.

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
						copy_member_to_value_array(i,prefix,write_index++,extent_index,i!=3);
				}else{

					for(uint16	i=1;i<=atom_count;++i)
						copy_member_to_value_array(i,prefix,write_index++,extent_index,!(!dereference_cptr	&&	i==1));
				}
			}
		}
	}

	void	Context::copy_member_to_value_array(uint16	child_index,bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr){

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
		case	Atom::R_PTR:
			overlay->values[write_index]=head;
			return;
		case	Atom::C_PTR:
			if(dereference_cptr){

				uint16	saved_index=index;
				index=_index;
				Context	cptr=**this;
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

	void	Context::getMember(void	*&object,uint32	&view_oid,ObjectType	&object_type,int16	&member_index)	const{

		if((*this)[0].getDescriptor()!=Atom::I_PTR){	//	ill-formed mod/set expression.

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		Context	cptr=Context(this->object,view,code,(*this)[0].asIndex(),overlay,data);	//	dereference manually (1 offset) to retain the cptr as is.

		if(cptr[0].getDescriptor()!=Atom::C_PTR){	//	ill-formed mod/set expression.

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		Context	c=*cptr.getChild(1);	//	this, vl_ptr, value_ptr or rptr.

		uint16	atom_count=cptr.getChildrenCount();
		for(uint16	i=2;i<atom_count;++i){	//	stop before the last iptr.

			if(cptr[i].getDescriptor()==Atom::VIEW)
				c=Context(c.getObject(),c.view,&c.view->code(0),0,NULL,VIEW);
			else
				c=*c.getChild(cptr[i].asIndex());
		}
		
		//	at this point, c is an iptr dereferenced to an object or a view; the next iptr holds the member index.
		//	c is pointing at the first atom of an object or a view.
		switch(c[0].getDescriptor()){
		case	Atom::OBJECT:
		case	Atom::MODEL:
		case	Atom::MARKER:
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::INSTANTIATED_CPP_PROGRAM:
			object_type=TYPE_OBJECT;
			object=c.getObject();
			break;
		case	Atom::GROUP:
		case	Atom::REDUCTION_GROUP:
			object_type=TYPE_GROUP;
			object=c.getObject();
			break;
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

	bool	Context::is_cmd_with_cptr()	const{

		if((*this)[0].getDescriptor()!=Atom::OBJECT)
			return	false;
		if((*this)[0].asOpcode()!=Opcodes::Cmd)
			return	false;
		if((*this)[2].atom!=EXECUTIVE_DEVICE)
			return	false;
		return	(*this)[1].asOpcode()==Opcodes::Mod	||
				(*this)[1].asOpcode()==Opcodes::Set	||
				(*this)[1].asOpcode()==Opcodes::Subst;
	}

	void	Context::trace()	const{

		std::cout<<"======== CONTEXT ========\n";
		switch(data){
		case	UNDEFINED:
			std::cout<<"undefined\n";
			return;
		case	MKS:
			std::cout<<"--> mks\n";
			return;
		case	VWS:
			std::cout<<"--> vws\n";
			return;
		}

		for(uint16	i=0;i<object->code_size();++i){

			if(index==i)
				std::cout<<">>";
			std::cout<<i<<"\t";
			code[i].trace();
			std::cout<<std::endl;
		}
	}

	uint16	Context::setAtomicResult(Atom	a)		const{	//	patch code with 32 bits data.
			
			overlay->patch_code(index,a);
			return	index;
		}

	uint16	Context::setTimestampResult(uint64	t)	const{	//	patch code with a VALUE_PTR
			
		overlay->patch_code(index,Atom::ValuePointer(overlay->values.size()));
		overlay->values.as_std()->resize(overlay->values.size()+3);
		uint16	value_index=overlay->values.size()-3;
		Utils::SetTimestamp(&overlay->values[value_index],t);
		return	value_index;
	}

	uint16	Context::setCompoundResultHead(Atom	a)	const{	//	patch code with a VALUE_PTR.
		
		uint16	value_index=overlay->values.size();
		overlay->patch_code(index,Atom::ValuePointer(value_index));
		addCompoundResultPart(a);
		return	value_index;
	}

	uint16	Context::addCompoundResultPart(Atom	a)	const{	//	store result in the value array.
		
		overlay->values.push_back(a);
		return	overlay->values.size()-1;
	}

	uint16	Context::addProduction(Code	*object,bool	check_for_existence)	const{	//	called by operators (ins and red).
		
		overlay->productions.push_back(_Mem::Get()->check_existence(object));
		return	overlay->productions.size()-1;
	}
}