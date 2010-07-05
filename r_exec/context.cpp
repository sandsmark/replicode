//	context.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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


using	namespace	r_code;

namespace	r_exec{

	bool	Context::operator	==(const	Context	&c)	const{

		Context	lhs=operator	*();
		Context	rhs=*c;

		if(lhs.data==REFERENCE	&&	lhs.index==0	&&
			rhs.data==REFERENCE	&&	rhs.index==0)	//	both point to an object's head, not one of its members.
			return	lhs.object==rhs.object;

		//	both contexts point to an atom which is not a pointer.
		if(lhs.head()!=rhs.head())
			return	false;

		if(lhs.head().isStructural()){	//	both are structural.

			uint16	atom_count=lhs.getChildrenCount();
			for(uint16	i=1;i<=atom_count;++i)
				if(*lhs.getChild(i)!=*rhs.getChild(i))
					return	false;
			return	true;
		}
		return	true;
	}

	bool	Context::match(const	Context	&input)	const{

		if(data==REFERENCE	&&	index==0	&&
			input.data==REFERENCE	&&	input.index==0)	//	both point to an object's head, not one of its members.
			return	object==input.object;

		if(code[index].isStructural()){

			uint16	atom_count=getChildrenCount();
			if(input.getChildrenCount()!=atom_count)
				return	false;

			for(uint16	i=1;i<=atom_count;++i){

				Context	pc=*getChild(i);
				Context	ic=*input.getChild(i);
				if(!pc.match(ic))
					return	false;
			}
			return	true;
		}

		switch(head().getDescriptor()){
		case	Atom::WILDCARD:
		case	Atom::T_WILDCARD:
			return	true;
		default:
			return	head().atom==input.head().atom;
		}
	}

	inline	bool	Context::operator	!=(const	Context	&c)	const{

		return	!(*this==c);
	}

	Context	Context::operator	*()	const{

		switch(head().getDescriptor()){
		case	Atom::VL_PTR:{	//	evaluate the code if necessary.
			
			Atom	a=code[head().asIndex()];
			uint8	d=a.getDescriptor();
			if(a.isStructural()	||	a.isPointer()	||
				(d!=Atom::VALUE_PTR	&&	d!=Atom::IPGM_PTR	&&	d!=Atom::IN_OBJ_PTR	&&	d!=Atom::IN_VW_PTR)){	//	the target location is not evaluated yet.

				Context	c(object,view,code,head().asIndex(),overlay,data);
				uint16	unused_index;
				if(c.evaluate_no_dereference(unused_index))
					return	c;	//	patched code: atom at VL_PTR's index changed to VALUE_PTR or 32 bits result.
				else	//	evaluation failed, return undefined context.
					return	Context();
			}else
				return	*Context(object,view,code,head().asIndex(),overlay,data);
		}case	Atom::I_PTR:
			if(code[head().asIndex()].isPointer()	&&	code[head().asIndex()].getDescriptor()!=Atom::C_PTR)
				return	*Context(object,view,code,head().asIndex(),overlay,data);
			return	Context(object,view,code,head().asIndex(),overlay,data);
		case	Atom::R_PTR:{

			Code	*o=object->get_reference(head().asIndex());
			return	Context(o,NULL,&o->code(0),0,NULL,REFERENCE);
		}case	Atom::C_PTR:{

			Context	c=*getChild(1);
			for(uint16	i=2;i<=getChildrenCount();++i)
				c=*c.getChild(code[index+i].asIndex());
			return	c;
		}
		case	Atom::THIS:	//	refers to the ipgm; the pgm view is not available.
			return	Context(overlay->getIPGM(),overlay->getIPGMView(),&overlay->getIPGM()->code(0),0,overlay,REFERENCE);
		case	Atom::VIEW:	//	never a reference, always in a cptr.
			if(overlay	&&	object==overlay->getIPGM())
				return	Context(object,view,&view->code(0),0,NULL,VIEW);
			return	*this;
		case	Atom::MKS:
			return	Context(object,MKS);
		case	Atom::VWS:
			return	Context(object,VWS);
		case	Atom::VALUE_PTR:
			return	Context(object,view,&overlay->values[0],head().asIndex(),overlay,data);
		case	Atom::IPGM_PTR:
			return	Context(overlay->getIPGM(),head().asIndex());
		case	Atom::IN_OBJ_PTR:
			return	Context(overlay->getInputObject(head().asViewIndex()),head().asIndex());
		case	Atom::IN_VW_PTR:
			return	Context(overlay->getInputObject(head().asViewIndex()),head().asIndex());
		case	Atom::PROD_PTR:
			return	Context(overlay->productions[head().asIndex()],0);
		default:
			return	*this;
		}
	}

	bool	Context::evaluate_no_dereference(uint16	&result_index)	const{

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

			uint16	atom_count=getChildrenCount();
			for(uint16	i=1;i<=atom_count;++i){

				uint16	unused_result_index;
				if(!(*getChild(i)).evaluate_no_dereference(unused_result_index))
					return	false;
			}
			return	Operator::Get(head().asOpcode())(*this,result_index);
		}case	Atom::MARKER:
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::GROUP:
		case	Atom::OBJECT:	//	incl. cmd.
			if(head().asOpcode()==Opcodes::PTN	||	head().asOpcode()==Opcodes::AntiPTN){	//	skip patterns.

				result_index=index;
				return	true;
			}
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

		Context	c=operator	*();
		//c.trace();
		return	c.evaluate_no_dereference(result_index);
	}

	void	Context::getChildAsMember(uint16	index,void	*&object,uint32	&view_oid,ObjectType	&object_type,uint16	&member_index)	const{

		if(head().getDescriptor()!=Atom::C_PTR){	//	ill-formed mod/set expression.

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		//	cptr is of one the following forms:
		//		cptr, this, iptr, ...
		//		cptr, vl_ptr, iptr, ...
		//		cptr, rptr, iptr, ...
		Context	c=*getChild(1);
		uint16	atom_count=getChildrenCount();
		for(uint16	i=2;i<atom_count;++i)	//	stop before the last iptr.
			c=*c.getChild((*this)[i].asIndex());
		
		//	at this point, c is an iptr dereferenced to an object or a view; the next iptr holds the member index.
		//	c is pointing at the first atom of an object or a view.
		switch(c.head().getDescriptor()){
		case	Atom::OBJECT:
		case	Atom::MARKER:
		case	Atom::INSTANTIATED_PROGRAM:
			object_type=TYPE_OBJECT;
			object=c.getObject();
			break;
		case	Atom::GROUP:
			object_type=TYPE_GROUP;
			object=c.getObject();
			break;
		case	Atom::S_SET:{	//	views are always copied; set the object to the view's group on which to perform an operation for the view's oid.

			object=c.view->get_host();
			view_oid=c[VIEW_OID].atom;	//	oid is hidden at the end of the view code; stored directly as a uint32.
			object_type=TYPE_VIEW;
			break;
		}default:	//	atomic value or ill-formed mod/set expression.
			object=NULL;
			object_type=TYPE_UNDEFINED;
			break;
		}

		//	get the index held by the last iptr.
		member_index=(*this)[atom_count].asIndex();
	}

	void	Context::trace()	const{

		std::cout<<"======== CONTEXT ========\n";
		switch(data){
		case	UNDEFINED:
			std::cout<<"undefined\n";
			return;
		case	VIEW:
			std::cout<<"--> vw\n";
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
}