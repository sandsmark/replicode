//	context.h
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
		Code	*object;			//	the object the code belongs to; unchanged when the context is dereferenced to the overlay's value array.
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

		void	addReference(Code	*destination,uint16	write_index,Code	*referenced_object)	const{

			destination->add_reference(referenced_object);
			destination->code(write_index)=Atom::RPointer(destination->references_size()-1);
		}

		void	addReference(View	*destination,uint16	write_index,Code	*referenced_object)	const{	//	view references are set in order (indices 0 then 1).

			uint16	r_ptr_index;
			if(destination->references[0])	//	first ref already in place
				r_ptr_index=1;
			else
				r_ptr_index=0;
			destination->references[r_ptr_index]=object->get_reference(head().asIndex());
			destination->code(write_index)=Atom::RPointer(r_ptr_index);
		}

		template<class	C>	void	copy_structure(C	*destination,uint16	write_index,uint16	&extent_index)	const{	//	assumes the context is a structure; C: Object or View.
			
			destination->code(write_index++)=head();

			uint16	atom_count=getChildrenCount();
			extent_index=write_index+atom_count;
			for(uint16	i=1;i<=atom_count;++i){

				Context	c=getChild(i);
				c.copy_member(destination,write_index++,extent_index);
			}
		}

		template<class	C>	void	copy_member(C	*destination,uint16	write_index,uint16	&extent_index)	const{

			switch(head().getDescriptor()){
			case	Atom::I_PTR:
			case	Atom::VL_PTR:
			case	Atom::C_PTR:
			case	Atom::VALUE_PTR:
			case	Atom::IPGM_PTR:
			case	Atom::IN_OBJ_PTR:
			case	Atom::IN_VW_PTR:
			case	Atom::THIS:{

				if(data==REFERENCE	&&	index==0)	//	points to an object, not to one of its members.
					addReference(destination,write_index,object);
				else{

					Context	c=operator	*();
					destination->code(write_index)=Atom::IPointer(extent_index);
					c.copy_structure(destination,extent_index,extent_index);
				}
				break;
			}
			case	Atom::R_PTR:
				addReference(destination,write_index,object->get_reference(head().asIndex()));
				break;
			case	Atom::PROD_PTR:
				addReference(destination,write_index,overlay->productions[head().asIndex()]);
				break;
			case	Atom::OPERATOR:
			case	Atom::OBJECT:
			case	Atom::MARKER:
			case	Atom::INSTANTIATED_PROGRAM:
			case	Atom::GROUP:
			case	Atom::SET:
			case	Atom::S_SET:
			case	Atom::STRING:
			case	Atom::TIMESTAMP:
				destination->code(write_index)=Atom::IPointer(extent_index);
				copy_structure(destination,extent_index,extent_index);
				break;
			default:
				destination->code(write_index)=head();
				break;
			}
		}
	public:
		static	Context	GetContextFromInput(View	*input,Overlay	*overlay){	return	Context(input->object,input,&input->object->code(0),0,overlay,REFERENCE);	}

		Context():object(NULL),view(NULL),code(NULL),index(0),overlay(NULL),data(UNDEFINED){}	//	undefined context (happens when accessing the view of an object when it has not been provided).
		Context(Code	*object,View	*view,Atom	*code,uint16	index,Overlay	*const	overlay,Data	data=ORIGINAL_PGM):object(object),view(view),code(code),index(index),overlay(overlay),data(data){}
		Context(Code	*object,uint16	index):object(object),view(NULL),code(&object->code(0)),index(index),overlay(NULL),data(REFERENCE){}
		Context(Code	*object,Data	data):object(object),view(NULL),code(&object->code(0)),index(0),overlay(NULL),data(data){}

		bool	evaluate(uint16	&result_index)						const;	//	index is set to the index of the result, undefined in case of failure.
		bool	evaluate_no_dereference(uint16	&result_index)		const;

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

		Atom	head()					const{	return	code[index];	}
		uint16	getChildrenCount()		const{	return	code[index].getAtomCount();	}
		Context	getChild(uint16	index)	const{
			
			switch(data){
			case	ORIGINAL_PGM:
				return	Context(object,view,code,this->index+index,overlay,ORIGINAL_PGM);
			case	REFERENCE:
				return	Context(object,view,code,this->index+index,overlay,REFERENCE);
			case	VIEW:
				return	Context(object,view,code,this->index+index,NULL,VIEW);
			case	MKS:{

				uint16	i=0;
				std::list<Code	*>::const_iterator	m;
				for(m=object->markers.begin();i<this->index+index-1;++m,++i);
				return	Context(*m,0);
			}case	VWS:
				return	Context();	//	never used for iterating views (unexposed).
			default:	//	undefined context.
				return	Context();
			}
		}
		Atom	&operator	[](uint16	i)	const{	return	code[index+i];	}
		Code	*getObject()				const{	return	object;	}
		uint16	getIndex()					const{	return	index;	}

		Context	operator	*()	const;

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

		uint16	addProduction(Code	*object)	const{
			
			overlay->productions.push_back(object);
			return	overlay->productions.size()-1;
		}

		template<class	C>	void	copy(C	*destination,uint16	write_index)	const{
			
			uint16	extent_index=0;
			copy_structure(destination,write_index,extent_index);
		}

		typedef	enum{
			TYPE_OBJECT=0,
			TYPE_VIEW=1,
			TYPE_GROUP=2,
			TYPE_UNDEFINED=3
		}ObjectType;

		//	To retrieve objects, groups and views in mod/set expressions; views are copied.
		void	getChildAsMember(uint16	index,void	*&object,uint32	&view_oid,ObjectType	&object_type,uint16	&member_index)	const;

		//	'this' is a context on a pattern skeleton.
		bool	match(const	Context	&input)	const;

		//	called by operators.
		r_code::Code	*buildObject(Atom	head)	const{	return	overlay->buildObject(head);	}

		void	trace()	const;
	};
}


#endif