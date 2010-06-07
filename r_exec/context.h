#ifndef	context_h
#define	context_h

#include	"atom.h"
#include	"../r_code/utils.h"
#include	"object.h"


namespace	r_exec{

	class	Overlay;

	//	Evaluation context.
	class	dll_export	Context{
	private:
		Object	*object;			//	the object the code belongs to; unchanged when dereferencing to the overlay's value array.
		View	*view;				//	the object's view, can be NULL when dereferenced to a reference_set or a marker_set.
		Atom	*code;				//	the object's code.
		uint16	index;				//	in the code;
		Overlay	*const	overlay;	//	the overlay where the evaluation is performed.
		typedef	enum{
			CODE=0,
			MKS=1,
			VWS=2,
			UNDEFINED=3
		}Data;
		Data	data;				//	when code==NULL, indicates whether the context refers to the mks or vws, or is undefined.

		void	addReference(Object	*destination,uint16	&index)	const{

			destination->reference_set.push_back(object->reference_set[code->asIndex()]);
			destination->code[index++]=Atom::RPointer(destination->reference_set.size()-1);
		}

		void	addReference(View	*destination,uint16	&index)	const{	//	view references are set in order (indices 0 then 1).

			uint16	r_ptr_index;
			if(destination->reference_set[0])	//	first ref already in place
				r_ptr_index=1;
			else
				r_ptr_index=0;
			destination->reference_set[r_ptr_index]=object->reference_set[code->asIndex()];
			destination->code[index++]=Atom::RPointer(r_ptr_index);
		}
	public:
		Context():object(NULL),view(NULL),code(NULL),index(0),overlay(NULL),data(UNDEFINED){}	//	undefined context (happens when accessing the view of an object when it has not been provided).
		Context(Object	*object,View	*view,Atom	*code,uint16	index,Overlay	*const	overlay):object(object),view(view),code(code),index(index),overlay(overlay),data(CODE){}
		Context(Object	*object,View	*view,Overlay	*const	overlay,Data	data):object(object),view(view),code(NULL),index(0),overlay(overlay),data(data){}

		bool	evaluate()	const;
		bool	evaluate_no_dereference()	const;

		Context	getChild(uint16	index)	const{
			switch(data){
			case	CODE:
				return	Context(object,view,code,index,overlay).dereference();
			case	MKS:
				return	Context(object->marker_set[index-1],NULL,&object->marker_set[index-1]->code[0],0,overlay);
			case	VWS:
				return	Context();	//	never used for iterating views.
			default:	//	undefined context.
				return	*this;
			}
		}

		uint16	getChildrenCount()	const{
			return	code->getAtomCount();
		}

		Context	&operator	=(Context	&c){
			object=c.object;
			view=c.view;
			code=c.code;
			index=c.index;
			data=c.data;
			return	*this;
		}

		bool	operator	==(const	Context	&c)	const;
		bool	operator	!=(const	Context	&c)	const;

		Atom	*getCode()		const{	return	code;	}
		Object	*getObject()	const{	return	object;	}
		uint16	getIndex()		const{	return	index;	}

		Context	dereference()	const;

		void	patch_code(uint16	location,Atom	value)	const{	overlay->patch_code(location,value);	}

		void	commit()	const{	overlay->commit();		}
		void	rollback()	const{	overlay->rollback();	}

		void	setAtomicResult(Atom	a)		const{	//	patch code with 32 bits data.
			if(code)
				overlay->patch_code(index,a);
		}

		void	setTimestampResult(uint64	t)	const{	//	patch code with a VALUE_PTR
			if(!code)
				return;
			overlay->patch_code(index,Atom::ValuePointer(overlay->values.size()));
			overlay->values.resize(overlay->values.size()+3);
			Timestamp::Set(&overlay->values[overlay->values.size()-3],t);
		}

		void	setCompoundResultHead(Atom	a)	const{	//	patch code with a VALUE_PTR.
			if(!code)
				return;
			overlay->patch_code(index,Atom::ValuePointer(overlay->values.size()));
			addCompoundResultPart(a);
		}

		void	addCompoundResultPart(Atom	a)	const{	//	store result in the value array.
			if(code)
				overlay->values.push_back(a);
		}

		uint16	addExplicitInstantiation(Object	*object)	const{
			overlay->explicit_instantiations.push_back(object);
			return	overlay->explicit_instantiations.size()-1;
		}

		template<class	C>	void	copy(C	*object,uint16	index)	const{	//	assumes the context is a structure; C: Object or View.
			uint16	extent;
			copy(object,index,extent);
		}

		template<class	C>	void	copy(C	*object,uint16	index,uint16	&extent)	const{	//	convenience.

			uint16	atom_count=code->getAtomCount();
			extent=index+atom_count+1;

			object->code[index++]=code[0];

			for(uint16	i=1;i<=atom_count;++i){

				Atom	a=code[i];
				switch(a.getDescriptor()){
				case	Atom::I_PTR:
				case	Atom::VL_PTR:
				case	Atom::C_PTR:
				case	Atom::VALUE_PTR:
				case	Atom::IPGM_PTR:
				case	Atom::INPUT_PTR:
				case	Atom::PROD_PTR:
				case	Atom::THIS:{

					Context	c=dereference();
					switch(c.code->getDescriptor()){
					case	Atom::OPERATOR:
					case	Atom::OBJECT:
					case	Atom::MARKER:
					case	Atom::SET:
					case	Atom::S_SET:
					case	Atom::STRING:
					case	Atom::TIMESTAMP:{

						uint16	new_extent;
						object->code[index++]=Atom::IPointer(extent);
						c.copy(object,extent,new_extent);
						extent=new_extent;
						break;
					}default:	//	fits in 32 bits
						object->code[index++]=c.code[0];
						break;
					}
					break;
				}case	Atom::R_PTR:
					addReference(object,index);
					break;
				default:
					object->code[index++]=a;
					break;
				}
			}
		}

		typedef	enum{
			TYPE_OBJECT=0,
			TYPE_VIEW=1,
			TYPE_GROUP=2,
			TYPE_UNDEFINED=3
		}ObjectType;

		void	getChildAsMember(uint16	index,void	*&object,ObjectType	object_type,uint16	member_index)	const;
	};
}


#endif