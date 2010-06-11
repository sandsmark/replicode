#include	"context.h"
#include	"pgm_overlay.h"
#include	"operator.h"


using	namespace	r_code;

namespace	r_exec{

	bool	Context::operator	==(const	Context	&c)	const{

		Context	lhs=dereference();
		Context	rhs=c.dereference();

		if(lhs.data==REFERENCE	&&	lhs.index==0	&&
			rhs.data==REFERENCE	&&	rhs.index==0)	//	both point to an object's head, not one of its members.
			return	lhs.object==rhs.object;

		//	both contexts point to an atom which is not a pointer.
		if(lhs[0]!=rhs[0])
			return	false;

		if(lhs[0].isStructural()){	//	both are structural.

			uint16	atom_count=lhs[0].getAtomCount();
			for(uint16	i=1;i<=atom_count;++i)
				if(lhs.getChild(i)!=rhs.getChild(i))
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

			uint16	atom_count=code[index].getAtomCount();
			if(input.getChildrenCount()!=atom_count)
				return	false;

			for(uint16	i=1;i<=atom_count;++i){

				Context	pc=getChild(i);
				Context	ic=input.getChild(i);
				if(!pc.match(ic))
					return	false;
			}
			return	true;
		}

		switch(code[index].getDescriptor()){
		case	Atom::WILDCARD:
		case	Atom::T_WILDCARD:
			return	true;
		default:
			return	(*this)[0].atom==input[0].atom;
		}
	}

	inline	bool	Context::operator	!=(const	Context	&c)	const{

		return	!(*this==c);
	}

	Context	Context::dereference()	const{

		if(!code)
			return	*this;

		switch(code[index].getDescriptor()){
		case	Atom::VL_PTR:{	//	evaluate the code if necessary.
			
			Atom	a=code[code[index].asIndex()];
			uint8	d=a.getDescriptor();
			if(a.isStructural()	||	a.isPointer()	||
				(d!=Atom::VALUE_PTR	&&	d!=Atom::IPGM_PTR	&&	d!=Atom::IN_OBJ_PTR	&&	d!=Atom::IN_VW_PTR)){	//	the target location is not evaluated yet.

				Context	c(object,view,code,code[index].asIndex(),overlay,data);
				if(c.evaluate_no_dereference())
					return	c;	//	patched code: atom at VL_PTR's index changed to VALUE_PTR or 32 bits result.
				else	//	evaluation failed, return undefined context.
					return	Context();
			}else
				return	Context(object,view,code,code[index].asIndex(),overlay,data).dereference();
		}case	Atom::I_PTR:
			return	Context(object,view,code,code[index].asIndex(),overlay,data).dereference();
		case	Atom::R_PTR:{

			Object	*o=object->reference_set[code[index].asIndex()];
			return	Context(o,NULL,&o->code[0],0,NULL,REFERENCE);
		}case	Atom::C_PTR:{

			Context	c=getChild(1).dereference();
			for(uint16	i=2;i<=code->getAtomCount();++i)
				c=c.getChild(code[index+i].asIndex()).dereference();
			return	c;
		}
		case	Atom::THIS:	//	refers to the ipgm; the pgm view is not available.
			return	Context(overlay->getIPGM(),overlay->getIPGMView(),&overlay->getIPGM()->code[0],0,NULL,REFERENCE);
		case	Atom::VIEW:	//	never a reference, always in a cptr.
			return	Context(object,view,&view->code[0],0,NULL,VIEW);
		case	Atom::MKS:
			return	Context(object,MKS);
		case	Atom::VWS:
			return	Context(object,VWS);
		case	Atom::VALUE_PTR:
			return	Context(object,view,&overlay->values[0],(*this)[0].asIndex(),overlay,data).dereference();
		case	Atom::IPGM_PTR:
			return	Context(overlay->getIPGM(),(*this)[0].asIndex()).dereference();
		case	Atom::IN_OBJ_PTR:
			return	Context(overlay->getInputObject((*this)[0].asViewIndex()),(*this)[0].asIndex()).dereference();
		case	Atom::IN_VW_PTR:
			return	Context(overlay->getInputObject((*this)[0].asViewIndex()),(*this)[0].asIndex()).dereference();
		case	Atom::PROD_PTR:
			return	Context(overlay->productions[(*this)[0].asIndex()],0).dereference();
		default:
			return	*this;
		}
	}

	bool	Context::evaluate_no_dereference()	const{

		if(!code)
			return	data!=UNDEFINED;
		switch(code[index].getDescriptor()){
		case	Atom::OPERATOR:{

			uint16	atom_count=(*this)[0].getAtomCount();
			for(uint16	i=1;i<=atom_count;++i){

				if(!getChild(i).evaluate())
					return	false;
			}
			return	Operator::Get((*this)[0].asOpcode())(*this);
		}case	Atom::OBJECT:	//	incl. cmd.
			if((*this)[0].asOpcode()==Object::PTNOpcode	||	(*this)[0].asOpcode()==Object::AntiPTNOpcode)	//	skip patterns.
				return	true;
		case	Atom::MARKER:
		case	Atom::SET:
		case	Atom::S_SET:{

			uint16	atom_count=(*this)[0].getAtomCount();
			for(uint16	i=1;i<=atom_count;++i){

				if(!getChild(i).evaluate())
					return	false;
			}
			return	true;
		}default:
			return	true;
		}
	}

	inline	bool	Context::evaluate()	const{

		if(!code)
			return	data!=UNDEFINED;
		Context	c=dereference();
		return	c.evaluate_no_dereference();
	}

	void	Context::getChildAsMember(uint16	index,void	*&object,ObjectType	object_type,uint16	member_index)	const{

		if((*this)[0].getDescriptor()!=Atom::C_PTR){	//	ill-formed mod/set expression.

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		//	cptr is of one the following forms:
		//		cptr, this, iptr, ...
		//		cptr, vl_ptr, iptr, ...
		//		cptr, rptr, iptr, ...
		Context	c=getChild(1).dereference();
		uint16	atom_count=(*this)[0].getAtomCount();
		for(uint16	i=2;i<atom_count;++i)	//	stop before the last iptr.
			c=c.getChild((*this)[i].asIndex()).dereference();
		
		// at this point, c is an iptr dereferenced to an object or a view; the next iptr holds the member index.
		switch(c[0].getDescriptor()){
		case	Atom::OBJECT:
			if(c[0].asOpcode()==Object::GroupOpcode){	//	must be a reference.

				object=c.getObject();
				object_type=TYPE_GROUP;
			}else{

				object=new	Object();
				c.copy((Object	*)object,0);	//	PB: not sure.
				object_type=TYPE_OBJECT;
			}
			break;
		case	Atom::VIEW:	//	PB: 
			object=new	View();
			c.copy((View	*)object,0);
			object_type=TYPE_VIEW;
			break;
		default:	//	atomic value or ill-formed mod/set expression.
			object=NULL;
			object_type=TYPE_UNDEFINED;
			break;
		}

		//	get the index held by the last iptr.
		c=c.getChild((*this)[atom_count].getAtomCount());
		member_index=c[0].asIndex();
	}
}