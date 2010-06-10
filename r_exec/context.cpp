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
		if(lhs.code[lhs.index]!=rhs.code[rhs.index])
			return	false;

		if(lhs.code[lhs.index].isStructural()){	//	both are structural.

			uint16	atom_count=lhs.code[lhs.index].getAtomCount();
			for(uint16	i=1;i<atom_count;++i)
				if(lhs.getChild(i)!=rhs.getChild(i))
					return	false;
			return	true;
		}
		return	true;
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
				c=c.getChild(code[i].asIndex()).dereference();
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
			return	Context(object,view,&overlay->values[0],code[index].asIndex(),overlay,data).dereference();
		case	Atom::IPGM_PTR:
			return	Context(overlay->getIPGM(),code[index].asIndex()).dereference();
		case	Atom::IN_OBJ_PTR:
			return	Context(overlay->getInputObject(code[index].asViewIndex()),code[index].asIndex()).dereference();
		case	Atom::IN_VW_PTR:
			return	Context(overlay->getInputObject(code[index].asViewIndex()),code[index].asIndex()).dereference();
		case	Atom::PROD_PTR:
			return	Context(overlay->explicit_instantiations[code->asIndex()],0).dereference();
		default:
			return	*this;
		}
	}

	bool	Context::evaluate_no_dereference()	const{

		if(!code)
			return	data!=UNDEFINED;
		switch(code[index].getDescriptor()){
		case	Atom::OPERATOR:{

			uint16	atom_count=code[index].getAtomCount();
			for(uint16	i=1;i<=atom_count;++i){

				if(!getChild(i).evaluate())
					return	false;
			}
			return	Operator::Get(code[index].asOpcode())(*this);
		}case	Atom::OBJECT:	//	incl. cmd.
			if(code[index].asOpcode()==Object::PTNOpcode	||	code[index].asOpcode()==Object::AntiPTNOpcode)	//	skip for patterns.
				return	true;
		case	Atom::MARKER:
		case	Atom::SET:
		case	Atom::S_SET:{

			uint16	atom_count=code[index].getAtomCount();
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

		if(code[index].getDescriptor()!=Atom::C_PTR){	//	ill-formed mod/set expression

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		Context	c=getChild(1).dereference();
		uint16	atom_count=code[index].getAtomCount();
		for(uint16	i=2;i<atom_count;++i)	//	stop before the last iptr.
			c=c.getChild(code[i].asIndex()).dereference();
		
		// at this point c is the object; the next iptr gives the member index.
		switch(c.code->getDescriptor()){
		case	Atom::OBJECT:
			if(c.code->asOpcode()==Object::GroupOpcode){	//	must be a reference.

				object=c.getObject();
				object_type=TYPE_GROUP;
			}else{

				object=new	Object();
				c.copy((Object	*)object,0);
				object_type=TYPE_OBJECT;
			}
			break;
		case	Atom::VIEW:
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
		c=c.getChild(code[atom_count].getAtomCount()).dereference();
		member_index=c.code[c.index].asIndex();
	}
}