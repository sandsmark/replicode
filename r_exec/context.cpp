#include	"context.h"
#include	"pgm_overlay.h"
#include	"operator.h"


using	namespace	r_code;

namespace	r_exec{

	bool	Context::operator	==(const	Context	&c)	const{

		if(code[0].isStructural()){

			if(!c.code[0].isStructural())
				return	false;
			if(code[0].getAtomCount()!=c.code[0].getAtomCount())
				return	false;
			for(uint16	i=0;i<code[0].getAtomCount();++i)
				if(getChild(i)!=c.getChild(i))
					return	false;
		}else
			return	code[0]==c.code[0];
	}

	inline	bool	Context::operator	!=(const	Context	&c)	const{

		return	!(*this==c);
	}

	Context	Context::dereference()	const{

		if(!code)
			return	*this;

		switch(code[0].getDescriptor()){
		case	Atom::I_PTR:
			return	Context(object,view,code,code[index].asIndex(),overlay).dereference();
		case	Atom::VL_PTR:{	//	evaluate the code if necessary.
			
			Atom	a=code[code[index].asIndex()];
			uint8	d=a.getDescriptor();
			if(a.isStructural()	||	a.isPointer()	||
				(d!=Atom::VALUE_PTR	&&	d!=Atom::IPGM_PTR	&&	d!=Atom::INPUT_PTR)){	//	the target location is not evaluated yet.

				Context	c(object,view,code,code[index].asIndex(),overlay);
				if(c.evaluate_no_dereference())
					return	c;	//	patched code: atom at VL_PTR's index changed to VALUE_PTR or 32 bits result.
				else	//	evaluation failed, return undefined context.
					return	Context();
			}
			return	Context(object,view,code,code[index].asIndex(),overlay).dereference();	//	code is already evaluated (and patched): do as for I_PTR.
		}case	Atom::R_PTR:{

			Object	*o=object->reference_set[code[index].asIndex()];
			return	Context(o,NULL,&o->code[0],0,overlay).dereference();
		}case	Atom::C_PTR:{

			Context	c=getChild(1).dereference();
			for(uint16	i=2;i<=code->getAtomCount();++i)
				c=c.getChild(code[i].asIndex()).dereference();
			return	c;
		}
		case	Atom::THIS:	//	refers to the ipgm. The pgm view is not available.
			return	Context(overlay->getIPGM(),overlay->getIPGMView(),&overlay->getIPGM()->code[0],0,overlay);
		case	Atom::VIEW:
			if(view)
				return	Context(object,view,&view->code[0],0,overlay);
			return	Context();
		case	Atom::MKS:
			return	Context(object,view,overlay,MKS);
		case	Atom::VWS:
			return	Context(object,view,overlay,VWS);
		case	Atom::VALUE_PTR:
			return	Context(object,view,&overlay->values[0],code[index].asIndex(),overlay).dereference();
		case	Atom::IPGM_PTR:
			return	Context(overlay->getIPGM(),overlay->getIPGMView(),&overlay->getIPGM()->code[0],code[index].asIndex(),overlay).dereference();
		case	Atom::INPUT_PTR:
			return	Context(overlay->getInput(code->asIndex()),overlay->getInputView(code[index].asIndex()),&overlay->getInput(code[index].getInputIndex())->code[0],code[index].asIndex(),overlay).dereference();
		case	Atom::PROD_PTR:
			return	Context(overlay->explicit_instantiations[code->asIndex()],NULL,&overlay->explicit_instantiations[code->asIndex()]->code[0],0,overlay).dereference();
		default:
			return	*this;
		}
	}

	bool	Context::evaluate_no_dereference()	const{

		if(!code)
			return	data!=UNDEFINED;
		switch(code[0].getDescriptor()){
		case	Atom::OPERATOR:
			for(uint16	i=1;i<=code->getAtomCount();++i)
				getChild(i).evaluate();
			return	Operator::Get(code->asOpcode())(*this);
		case	Atom::OBJECT:	//	incl. cmd.
			if(code->asOpcode()==Object::PTNOpcode	||	code->asOpcode()==Object::AntiPTNOpcode)	//	skip for patterns.
				return	true;
		case	Atom::MARKER:
		case	Atom::SET:
		case	Atom::S_SET:{

			uint16	atom_count=code->getAtomCount();
			for(uint16	i=1;i<=atom_count;++i)
				if(!getChild(i).evaluate())
					return	false;
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

		if(code[index].getDescriptor()!=Atom::C_PTR){

			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		Context	c=getChild(1).dereference();
		for(uint16	i=2;i<code->getAtomCount();++i)
			c=c.getChild(code[i].asIndex()).dereference();
		
		// at this point c is the object; the next index is the member index.
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
		default:
			object=NULL;
			object_type=TYPE_UNDEFINED;
			member_index=0;
			return;
		}

		c=c.getChild(code->getAtomCount()).dereference();
		member_index=c.code[0].asFloat();
	}
}