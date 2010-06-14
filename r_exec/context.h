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
		Object	*object;			//	the object the code belongs to; unchanged when the context is dereferenced to the overlay's value array.
		View	*view;				//	the object's view, can be NULL when the context is dereferenced to a reference_set or a marker_set.
		Atom	*code;				//	the object's code, or the code in value array, or the view's code when the context is dereferenced from Atom::VIEW.
		uint16	index;				//	in the code;
		Overlay	*const	overlay;	//	the overlay where the evaluation is performed; NULL when the context is dereferenced outside the original pgm or outside the value array.
		typedef	enum{				//	indicates whether the context refers to:
			ORIGINAL_PGM=0,			//		- the pgm being reducing inputs;
			REFERENCE=1,			//		- a reference to another object;
			VIEW=2,					//		- a view;
			MKS=3,					//		- the mks or an object;
			VWS=4,					//		- the vws of an object;
			UNDEFINED=5
		}Data;
		Data	data;

		void	addReference(Object	*destination,uint16	&write_index)	const{

			destination->reference_set.push_back(object->reference_set[code[index].asIndex()]);
			destination->code[write_index++]=Atom::RPointer(destination->reference_set.size()-1);
		}

		void	addReference(View	*destination,uint16	&write_index)	const{	//	view references are set in order (indices 0 then 1).

			uint16	r_ptr_index;
			if(destination->reference_set[0])	//	first ref already in place
				r_ptr_index=1;
			else
				r_ptr_index=0;
			destination->reference_set[r_ptr_index]=object->reference_set[code[index].asIndex()];
			destination->code[write_index++]=Atom::RPointer(r_ptr_index);
		}
	public:
		static	Context	GetContextFromInput(View	*input,Overlay	*overlay){	return	Context((r_exec::Object	*)input->object,input,&input->object->code[0],0,overlay,REFERENCE);	}

		Context():object(NULL),view(NULL),code(NULL),index(0),overlay(NULL),data(UNDEFINED){}	//	undefined context (happens when accessing the view of an object when it has not been provided).
		Context(Object	*object,View	*view,Atom	*code,uint16	index,Overlay	*const	overlay,Data	data=ORIGINAL_PGM):object(object),view(view),code(code),index(index),overlay(overlay),data(data){}
		Context(Object	*object,uint16	index):object(object),view(NULL),code(&object->code[0]),index(index),overlay(NULL),data(REFERENCE){}

		bool	evaluate(uint16	&index)					const;	//	index is set to the index of the result, undefined in case of failure.
		bool	evaluate_no_dereference(uint16	&index)	const;

		Context	getChild(uint16	index)	const{
			switch(data){
			case	ORIGINAL_PGM:
				return	Context(object,view,code,index,overlay,ORIGINAL_PGM).dereference();
			case	REFERENCE:
				return	Context(object,NULL,code,index,NULL,REFERENCE).dereference();
			case	VIEW:
				return	Context(object,view,code,index,NULL,VIEW).dereference();
			case	MKS:
				return	Context(object->marker_set[index-1],0);
			case	VWS:
				return	Context();	//	never used for iterating views (unexposed).
			default:	//	undefined context.
				return	Context();
			}
		}

		uint16	getChildrenCount()	const{
			return	code[index].getAtomCount();
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

		Atom	&operator	[](uint16	i)	const{	return	code[index+1];	}
		Object	*getObject()				const{	return	object;	}
		uint16	getIndex()					const{	return	index;	}

		Context	dereference()	const;

		void	patch_code(uint16	location,Atom	value)						const{	overlay->patch_code(location,value);	}
		void	patch_input_code(uint16	pgm_code_index,uint16	input_index)	const{	overlay->patch_input_code(pgm_code_index,input_index,0);	}

		void	commit()	const{	overlay->commit();		}
		void	rollback()	const{	overlay->rollback();	}

		uint16	setAtomicResult(Atom	a)		const{	//	patch code with 32 bits data.
			overlay->patch_code(index,a);
			return	index;
		}

		uint16	setTimestampResult(uint64	t)	const{	//	patch code with a VALUE_PTR
			overlay->patch_code(index,Atom::ValuePointer(overlay->values.size()));
			overlay->values.resize(overlay->values.size()+3);
			uint16	index=overlay->values.size()-3;
			Timestamp::Set(&overlay->values[index],t);
			return	index;
		}

		uint16	setCompoundResultHead(Atom	a)	const{	//	patch code with a VALUE_PTR.
			uint16	index=overlay->values.size();
			overlay->patch_code(index,Atom::ValuePointer(index));
			addCompoundResultPart(a);
			return	index;
		}

		uint16	addCompoundResultPart(Atom	a)	const{	//	store result in the value array.
			overlay->values.push_back(a);
			return	overlay->values.size()-1;
		}

		uint16	addProduction(Object	*object)	const{
			overlay->productions.push_back(object);
			return	overlay->productions.size()-1;
		}

		template<class	C>	void	copy(C	*destination,uint16	write_index)	const{	//	assumes the context is a structure; C: Object or View.
			uint16	extent_index;
			copy(destination,write_index,extent_index);
		}

		template<class	C>	void	copy(C	*destination,uint16	_write_index,uint16	&extent_index)	const{	//	convenience.

			uint16	write_index=_write_index;

			switch(code[index].getDescriptor()){
			case	Atom::I_PTR:
			case	Atom::VL_PTR:
			case	Atom::C_PTR:
			case	Atom::VALUE_PTR:
			case	Atom::IPGM_PTR:
			case	Atom::IN_OBJ_PTR:
			case	Atom::IN_VW_PTR:
			case	Atom::VIEW:
			case	Atom::THIS:{

				Context	c=dereference();
				if(c.data==REFERENCE	&&	c.index==0)	//	c points to an object, not to one of its members.
					c.addReference(destination,write_index);
				else
					c.copy(destination,write_index,extent_index);
				break;
			}
			case	Atom::R_PTR:
			case	Atom::PROD_PTR:
				addReference(destination,write_index);
				break;
			case	Atom::OPERATOR:
			case	Atom::OBJECT:
			case	Atom::MARKER:
			case	Atom::SET:
			case	Atom::S_SET:
			case	Atom::STRING:
			case	Atom::TIMESTAMP:{

				uint16	atom_count=code[index].getAtomCount();
				extent_index=write_index+atom_count+1;

				destination->code[write_index++]=Atom::IPointer(extent_index);
				destination->code[extent_index++]=code[index];

				uint16	new_extent_index;
				for(uint16	i=1;i<=atom_count;++i){

					Context	c=getChild(i);
					c.copy(destination,extent_index++,new_extent_index);
				}
				extent_index=new_extent_index;
				break;
			}
			default:
				destination->code[write_index]=code[index];
				break;
			}
		}

		typedef	enum{
			TYPE_OBJECT=0,
			TYPE_VIEW=1,
			TYPE_GROUP=2,
			TYPE_UNDEFINED=3
		}ObjectType;

		//	To retrieve objects, groups and views in mod/set expressions; views are copied.
		void	getChildAsMember(uint16	index,void	*&object,uint32	&view_oid,ObjectType	object_type,uint16	member_index)	const;

		//	'this' is a context on a pattern skeleton.
		bool	match(const	Context	&input)	const;
	};
}


#endif