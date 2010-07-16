//	context.h
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
			VALUE_ARRAY=5,			//		- code in the value array.
			UNDEFINED=6
		}Data;
		Data	data;

		bool	is_mod_or_set()	const;

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
			destination->references[r_ptr_index]=referenced_object;
			destination->code(write_index)=Atom::RPointer(r_ptr_index);
		}

		template<class	C>	void	copy_structure(C	*destination,uint16	write_index,uint16	&extent_index,bool	dereference_cptr)	const{	//	assumes the context is a structure; C: Object or View.
			
			destination->code(write_index++)=(*this)[0];

			uint16	atom_count=getChildrenCount();
			extent_index=write_index+atom_count;

			switch((*this)[0].getDescriptor()){
			case	Atom::C_PTR:	//	copy members as is (no dereference).
			case	Atom::TIMESTAMP:
				for(uint16	i=1;i<=atom_count;++i)
					destination->code(write_index++)=(*this)[i];
				break;
			default:
				if(is_mod_or_set()){

					for(uint16	i=1;i<=atom_count;++i){

						Context	c=getChild(i);
						c.copy_member(destination,write_index++,extent_index,i!=3);
					}
				}else{

					for(uint16	i=1;i<=atom_count;++i){

						Context	c=getChild(i);
						c.copy_member(destination,write_index++,extent_index,!(!dereference_cptr	&&	i==1));
					}
				}
				break;
			}
		}

		template<class	C>	void	copy_member(C	*destination,uint16	write_index,uint16	&extent_index,bool	dereference_cptr)	const{

			switch((*this)[0].getDescriptor()){
			case	Atom::I_PTR:
			case	Atom::VALUE_PTR:
			case	Atom::IPGM_PTR:{

				if(data==REFERENCE	&&	index==0)	//	points to an object, not to one of its members.
					addReference(destination,write_index,object);
				else{

					if(!dereference_cptr	&&	code[(*this)[0].asIndex()].getDescriptor()==Atom::C_PTR){

						Context	c=Context(object,view,code,(*this)[0].asIndex(),overlay,data);
						destination->code(write_index)=Atom::IPointer(extent_index);
						c.copy_structure(destination,extent_index,extent_index,dereference_cptr);
					}else{

						Context	c=**this;
						if(c[0].isStructural()){

							destination->code(write_index)=Atom::IPointer(extent_index);
							c.copy_structure(destination,extent_index,extent_index,dereference_cptr);
						}else
							destination->code(write_index)=c[0];
					}
				}
				break;
			}
			case	Atom::VL_PTR:
				if(code[(*this)[0].asIndex()].getDescriptor()==Atom::PROD_PTR){

					addReference(destination,write_index,overlay->productions[code[(*this)[0].asIndex()].asIndex()]);
					break;
				}
				if(code[(*this)[0].asIndex()].getDescriptor()==Atom::I_PTR){

					if(code[code[(*this)[0].asIndex()].asIndex()].getDescriptor()==Atom::IN_OBJ_PTR){

						addReference(destination,write_index,((IOverlay	*)overlay)->getInputObject(code[code[(*this)[0].asIndex()].asIndex()].asInputIndex()));
						break;
					}
				}
				(**this).copy_member(destination,write_index,extent_index,dereference_cptr);
				break;
			case	Atom::R_PTR:
				addReference(destination,write_index,object->get_reference((*this)[0].asIndex()));
				break;
			case	Atom::PROD_PTR:
				addReference(destination,write_index,overlay->productions[(*this)[0].asIndex()]);
				break;
			case	Atom::IN_OBJ_PTR:
				addReference(destination,write_index,((IOverlay	*)overlay)->getInputObject((*this)[0].asIndex()));
				break;
			case	Atom::OPERATOR:
			case	Atom::OBJECT:
			case	Atom::MARKER:
			case	Atom::INSTANTIATED_PROGRAM:
			case	Atom::GROUP:
			case	Atom::SET:
			case	Atom::S_SET:
			case	Atom::STRING:
				destination->code(write_index)=Atom::IPointer(extent_index);
				copy_structure(destination,extent_index,extent_index,dereference_cptr);
				break;
			default:
				destination->code(write_index)=(*this)[0];
				break;
			}
		}

		void	Context::copy_structure_to_value_array(bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr);
		void	Context::copy_member_to_value_array(uint16	child_index,bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr);
	public:
		static	Context	GetContextFromInput(View	*input,Overlay	*overlay){	return	Context(input->object,input,&input->object->code(0),0,overlay,REFERENCE);	}

		Context():object(NULL),view(NULL),code(NULL),index(0),overlay(NULL),data(UNDEFINED){}	//	undefined context (happens when accessing the view of an object when it has not been provided).
		Context(Code	*object,View	*view,Atom	*code,uint16	index,Overlay	*const	overlay,Data	data=ORIGINAL_PGM):object(object),view(view),code(code),index(index),overlay(overlay),data(data){}
		Context(Code	*object,uint16	index):object(object),view(NULL),code(&object->code(0)),index(index),overlay(NULL),data(REFERENCE){}
		Context(Code	*object,Data	data):object(object),view(NULL),code(&object->code(0)),index(0),overlay(NULL),data(data){}

		bool	evaluate(uint16	&result_index)					const;	//	index is set to the index of the result, undefined in case of failure.
		bool	evaluate_no_dereference(uint16	&result_index)	const;

		Context	&operator	=(const	Context	&c){
			
			object=c.object;
			view=c.view;
			code=c.code;
			index=c.index;
			data=c.data;
			return	*this;
		}

		bool	operator	==(const	Context	&c)	const;
		bool	operator	!=(const	Context	&c)	const;

		uint16	getChildrenCount()		const{
			
			uint16	c;
			switch(data){
			case	MKS:
				object->acq_markers();
				c=object->markers.size();
				object->rel_markers();
				return	c;
			case	VWS:
				object->acq_views();
				c=object->views.size();
				object->rel_views();
				return	c;
			default:
				return	code[index].getAtomCount();
			}
		}
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
				object->acq_markers();
				for(m=object->markers.begin();i<index-1;++i,++m){

					if(m==object->markers.end()){	//	happens when the list has changed after the call to getChildrenCount.

						object->rel_markers();
						return	Context();
					}
				}
				object->rel_markers();
				return	Context(*m,0);
			}case	VWS:{

				uint16	i=0;
				UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	v;
				object->acq_views();
				for(v=object->views.begin();i<index-1;++i,++v){

					if(v==object->views.end()){	//	happens when the list has changed after the call to getChildrenCount.

						object->rel_views();
						return	Context();
					}
				}
				object->rel_views();
				return	Context(object,(r_exec::View*)*v,&(*v)->code(0),this->index+index,NULL,VIEW);
			}case	VALUE_ARRAY:
				return	Context(object,view,code,this->index+index,overlay,VALUE_ARRAY);
			default:	//	undefined context.
				return	Context();
			}
		}
		Atom	&operator	[](uint16	i)	const{	return	code[index+i];	}
		Code	*getObject()				const{	return	object;	}
		uint16	getIndex()					const{	return	index;	}

		Context	operator	*()	const;
		void	dereference_once();
		bool	is_reference()	const{	return	data==REFERENCE;	}
		bool	is_undefined()	const{	return	data==UNDEFINED;	}

		void	patch_code(uint16	location,Atom	value)						const{	overlay->patch_code(location,value);	}
		void	patch_input_code(uint16	pgm_code_index,uint16	input_index)	const{	overlay->patch_input_code(pgm_code_index,input_index,0);	}

		void	commit()	const{	overlay->commit();		}
		void	rollback()	const{	overlay->rollback();	}
		uint16	get_last_patch_index()	const{	return	overlay->get_last_patch_index();	}
		void	unpatch_code(uint16	patch_index)	const{	overlay->unpatch_code(patch_index);	}

		uint16	setAtomicResult(Atom	a)		const{	//	patch code with 32 bits data.
			
			overlay->patch_code(index,a);
			return	index;
		}

		uint16	setTimestampResult(uint64	t)	const{	//	patch code with a VALUE_PTR
			
			overlay->patch_code(index,Atom::ValuePointer(overlay->values.size()));
			overlay->values.as_std()->resize(overlay->values.size()+3);
			uint16	value_index=overlay->values.size()-3;
			Timestamp::Set(&overlay->values[value_index],t);
			return	value_index;
		}

		uint16	setCompoundResultHead(Atom	a)	const{	//	patch code with a VALUE_PTR.
			
			uint16	value_index=overlay->values.size();
			overlay->patch_code(index,Atom::ValuePointer(value_index));
			addCompoundResultPart(a);
			return	value_index;
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
			copy_structure(destination,write_index,extent_index,true);
		}

		template<class	C>	void	copy(C	*destination,uint16	write_index,uint16	&extent_index)	const{
			
			copy_structure(destination,write_index,extent_index,true);
		}

		void	copy_to_value_array(uint16	&position);

		typedef	enum{
			TYPE_OBJECT=0,
			TYPE_VIEW=1,
			TYPE_GROUP=2,
			TYPE_UNDEFINED=3
		}ObjectType;

		//	To retrieve objects, groups and views in mod/set expressions; views are copied.
		void	getMember(void	*&object,uint32	&view_oid,ObjectType	&object_type,int16	&member_index)	const;

		//	'this' is a context on a pattern skeleton.
		bool	match(const	Context	&input)	const;

		//	called by operators.
		r_code::Code	*buildObject(Atom	head)	const{	return	overlay->buildObject(head);	}
		uint64			now()	const{	return	overlay->now();	}

		void	trace()	const;
	};
}


#endif