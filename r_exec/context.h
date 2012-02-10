//	ipgm_context.h
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

#ifndef	ipgm_context_h
#define	ipgm_context_h

#include	"../r_code/atom.h"
#include	"../r_code/utils.h"
#include	"object.h"
#include	"_context.h"
#include	"pgm_overlay.h"


namespace	r_exec{

	class	InputLessPGMOverlay;

	//	Evaluation context.
	class	dll_export	IPGMContext:
	public	_Context{
	private:
		Code	*object;			//	the object the code belongs to; unchanged when the context is dereferenced to the overlay's value array.
		View	*view;				//	the object's view, can be NULL when the context is dereferenced to a reference_set or a marker_set.

		bool	is_cmd_with_cptr()	const;

		void	addReference(Code	*destination,uint16	write_index,Code	*referenced_object)	const{

			for(uint16	i=0;i<destination->references_size();++i)
				if(referenced_object==destination->get_reference(i)){

					destination->code(write_index)=Atom::RPointer(i);
					return;
				}

			destination->add_reference(referenced_object);
			destination->code(write_index)=Atom::RPointer(destination->references_size()-1);
		}

		void	addReference(View	*destination,uint16	write_index,Code	*referenced_object)	const{	//	view references are set in order (index 0 then 1).

			uint16	r_ptr_index;
			if(destination->references[0])	//	first ref already in place.
				r_ptr_index=1;
			else
				r_ptr_index=0;
			destination->references[r_ptr_index]=referenced_object;
			destination->code(write_index)=Atom::RPointer(r_ptr_index);
		}

		template<class	C>	void	copy_structure(C		*destination,
													uint16	write_index,
													uint16	&extent_index,
													bool	dereference_cptr,
													int32	pgm_index)	const{	//	assumes the context is a structure; C: Object or View.
			
			if((*this)[0].getDescriptor()==Atom::OPERATOR	&&	Operator::Get((*this)[0].asOpcode()).is_syn()){	// (\ (expression ...)).
				
				IPGMContext	c=getChild(1);
				c.copy_member(destination,write_index,extent_index,dereference_cptr,pgm_index);
			}else{

				destination->code(write_index++)=(*this)[0];

				uint16	atom_count=getChildrenCount();
				extent_index=write_index+atom_count;

				switch((*this)[0].getDescriptor()){
				case	Atom::C_PTR:	// copy members as is (no dereference).
					if((*this)[1].getDescriptor()==Atom::VL_PTR){

						Atom	a=code[(*this)[1].asIndex()];
						if(a.getDescriptor()==Atom::I_PTR)
							a=code[a.asIndex()];
						switch(a.getDescriptor()){
						case	Atom::OUT_OBJ_PTR:
							destination->code(write_index++)=Atom::VLPointer(code[(*this)[1].asIndex()].asIndex());
							break;
						case	Atom::IN_OBJ_PTR:	//	TMP: assumes destination is a mk.rdx.
							destination->code(write_index++)=Atom::RPointer(1+a.asInputIndex());
							break;
						default:
							destination->code(write_index++)=(*this)[1];
							break;
						}
					}else
						destination->code(write_index++)=(*this)[1];
					for(uint16	i=2;i<=atom_count;++i)
						destination->code(write_index++)=(*this)[i];
					break;
				case	Atom::TIMESTAMP:	// copy members as is (no dereference).
					for(uint16	i=1;i<=atom_count;++i)
						destination->code(write_index++)=(*this)[i];
					break;
				default:
					if(is_cmd_with_cptr()){

						for(uint16	i=1;i<=atom_count;++i){

							IPGMContext	c=getChild(i);
							c.copy_member(destination,write_index++,extent_index,i!=2,pgm_index);
						}
					}else{	// if a pgm is being copied, indicate the starting index of the pgm so that we can turn on code patching and know if a cptr is referencing code inside the pgm (in that case it will not be dereferenced).

						int32	_pgm_index;
						if(pgm_index>=0)
							_pgm_index=pgm_index;
						else	if((*this)[0].getDescriptor()==Atom::OBJECT	&&	((*this)[0].asOpcode()==Opcodes::Pgm	||	(*this)[0].asOpcode()==Opcodes::AntiPgm))
							_pgm_index=index;
						else
							_pgm_index=-1;

						for(uint16	i=1;i<=atom_count;++i){

							IPGMContext	c=getChild(i);
							c.copy_member(destination,write_index++,extent_index,!(!dereference_cptr	&&	i==1),_pgm_index);
						}
					}
					break;
				}
			}
		}

		template<class	C>	void	copy_member(C		*destination,
												uint16	write_index,
												uint16	&extent_index,
												bool	dereference_cptr,
												int32	pgm_index)	const{

			switch((*this)[0].getDescriptor()){
			case	Atom::I_PTR:
			case	Atom::VALUE_PTR:
			case	Atom::IPGM_PTR:{

				if(data==REFERENCE	&&	index==0){	//	points to an object, not to one of its members.

					addReference(destination,write_index,object);
					break;
				}
				
				if(code[(*this)[0].asIndex()].getDescriptor()==Atom::C_PTR){

					if(!dereference_cptr	||	(pgm_index>0	&&	code[(*this)[0].asIndex()+1].asIndex()>pgm_index)){	// the latter case occurs when a cptr references code inside a pgm being copied.

						IPGMContext	c=IPGMContext(object,view,code,(*this)[0].asIndex(),(InputLessPGMOverlay	*)overlay,data);
						destination->code(write_index)=Atom::IPointer(extent_index);
						c.copy_structure(destination,extent_index,extent_index,dereference_cptr,pgm_index);
						break;
					}
				}

				IPGMContext	c=**this;
				if(c[0].isStructural()){

					destination->code(write_index)=Atom::IPointer(extent_index);
					if(pgm_index>0	&&	index>pgm_index	&&	data==STEM)
						this->patch_code(index,Atom::OutObjPointer(write_index));
					c.copy_structure(destination,extent_index,extent_index,dereference_cptr,pgm_index);
				}else
					destination->code(write_index)=c[0];
				break;
			}
			case	Atom::VL_PTR:
				switch(code[(*this)[0].asIndex()].getDescriptor()){
				case	Atom::PROD_PTR:
					addReference(destination,write_index,((InputLessPGMOverlay	*)overlay)->productions[code[(*this)[0].asIndex()].asIndex()]);
					break;
				case	Atom::I_PTR:
					if(code[code[(*this)[0].asIndex()].asIndex()].getDescriptor()==Atom::IN_OBJ_PTR)
						addReference(destination,write_index,((PGMOverlay	*)overlay)->getInputObject(code[code[(*this)[0].asIndex()].asIndex()].asInputIndex()));
					else
						(**this).copy_member(destination,write_index,extent_index,dereference_cptr,pgm_index);
					break;
				case	Atom::OUT_OBJ_PTR:
					destination->code(write_index)=Atom::VLPointer(code[(*this)[0].asIndex()].asIndex());
					break;
				case	Atom::IN_OBJ_PTR:
				case	Atom::D_IN_OBJ_PTR:{

					IPGMContext	c=**this;
					if(c.index==0)
						addReference(destination,write_index,c.object);
					else	if(c[0].isStructural()){

						destination->code(write_index++)=Atom::IPointer(extent_index);
						c.copy_structure(destination,extent_index,extent_index,dereference_cptr,pgm_index);
					}else
						c.copy_member(destination,write_index,extent_index,dereference_cptr,pgm_index);
					break;
				}default:
					(**this).copy_member(destination,write_index,extent_index,dereference_cptr,pgm_index);
					break;
				}
				break;
			case	Atom::R_PTR:
				addReference(destination,write_index,object->get_reference((*this)[0].asIndex()));
				break;
			case	Atom::PROD_PTR:
				addReference(destination,write_index,((InputLessPGMOverlay	*)overlay)->productions[(*this)[0].asIndex()]);
				break;
			case	Atom::IN_OBJ_PTR:
				addReference(destination,write_index,((PGMOverlay	*)overlay)->getInputObject((*this)[0].asIndex()));
				break;
			case	Atom::D_IN_OBJ_PTR:{
				IPGMContext	c=**this;
				addReference(destination,write_index,c.object);
				break;
			}case	Atom::OPERATOR:
			case	Atom::OBJECT:
			case	Atom::MARKER:
			case	Atom::INSTANTIATED_PROGRAM:
			case	Atom::INSTANTIATED_CPP_PROGRAM:
			case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
			case	Atom::INSTANTIATED_ANTI_PROGRAM:
			case	Atom::COMPOSITE_STATE:
			case	Atom::MODEL:
			case	Atom::GROUP:
			case	Atom::SET:
			case	Atom::S_SET:
			case	Atom::TIMESTAMP:
			case	Atom::STRING:
				destination->code(write_index)=Atom::IPointer(extent_index);
				if(pgm_index>0	&&	index>pgm_index	&&	data==STEM)
					this->patch_code(index,Atom::OutObjPointer(write_index));
				copy_structure(destination,extent_index,extent_index,dereference_cptr,pgm_index);
				break;
			case	Atom::T_WILDCARD:
				destination->code(write_index)=(*this)[0];
				break;
			default:
				destination->code(write_index)=(*this)[0];
				if(pgm_index>0	&&	index>pgm_index	&&	data==STEM)
					this->patch_code(index,Atom::OutObjPointer(write_index));
				break;
			}
		}

		void	IPGMContext::copy_structure_to_value_array(bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr);
		void	IPGMContext::copy_member_to_value_array(uint16	child_index,bool	prefix,uint16	write_index,uint16	&extent_index,bool	dereference_cptr);
	public:
		static	IPGMContext	GetContextFromInput(View	*input,InputLessPGMOverlay	*overlay){	return	IPGMContext(input->object,input,&input->object->code(0),0,overlay,REFERENCE);	}

		IPGMContext():_Context(NULL,0,NULL,UNDEFINED),object(NULL),view(NULL){}	//	undefined context (happens when accessing the view of an object when it has not been provided).
		IPGMContext(Code	*object,View	*view,Atom	*code,uint16	index,InputLessPGMOverlay	*const	overlay,Data	data=STEM):_Context(code,index,overlay,data),object(object),view(view){}
		IPGMContext(Code	*object,uint16	index):_Context(&object->code(0),index,NULL,REFERENCE),object(object),view(NULL){}
		IPGMContext(Code	*object,Data	data):_Context(&object->code(0),index,NULL,data),object(object),view(NULL){}

		//	_Context implementation.
		_Context	*assign(const	_Context	*c){

			IPGMContext	*_c=new	IPGMContext(*(IPGMContext	*)c);
			return	_c;
		}

		bool	equal(const	_Context	*c)	const{	return	*this==*(IPGMContext	*)c;	}

		Atom	&get_atom(uint16	i)	const{	return	this->operator	[](i);	}

		uint16	get_object_code_size()	const{	return	object->code_size();	}

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
		_Context	*_getChild(uint16	index)	const{

			IPGMContext	*_c=new	IPGMContext(getChild(index));
			return	_c;
		}
		
		_Context	*dereference()	const{

			IPGMContext	*_c=new	IPGMContext(**this);
			return	_c;
		}

		//	IPGM specifics.
		bool	evaluate(uint16	&result_index)					const;	//	index is set to the index of the result, undefined in case of failure.
		bool	evaluate_no_dereference(uint16	&result_index)	const;

		IPGMContext	&operator	=(const	IPGMContext	&c){
			
			object=c.object;
			view=c.view;
			code=c.code;
			index=c.index;
			data=c.data;
			return	*this;
		}

		bool	operator	==(const	IPGMContext	&c)	const;
		bool	operator	!=(const	IPGMContext	&c)	const;

		IPGMContext	getChild(uint16	index)	const{
			
			switch(data){
			case	STEM:
				return	IPGMContext(object,view,code,this->index+index,(InputLessPGMOverlay	*)overlay,STEM);
			case	REFERENCE:
				return	IPGMContext(object,view,code,this->index+index,(InputLessPGMOverlay	*)overlay,REFERENCE);
			case	VIEW:
				return	IPGMContext(object,view,code,this->index+index,NULL,VIEW);
			case	MKS:{

				uint16	i=0;
				std::list<Code	*>::const_iterator	m;
				object->acq_markers();
				for(m=object->markers.begin();i<index-1;++i,++m){

					if(m==object->markers.end()){	//	happens when the list has changed after the call to getChildrenCount.

						object->rel_markers();
						return	IPGMContext();
					}
				}
				object->rel_markers();
				return	IPGMContext(*m,0);
			}case	VWS:{

				uint16	i=0;
				UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	v;
				object->acq_views();
				for(v=object->views.begin();i<index-1;++i,++v){

					if(v==object->views.end()){	//	happens when the list has changed after the call to getChildrenCount.

						object->rel_views();
						return	IPGMContext();
					}
				}
				object->rel_views();
				return	IPGMContext(object,(r_exec::View*)*v,&(*v)->code(0),this->index+index,NULL,VIEW);
			}case	VALUE_ARRAY:
				return	IPGMContext(object,view,code,this->index+index,(InputLessPGMOverlay	*)overlay,VALUE_ARRAY);
			default:	//	undefined context.
				return	IPGMContext();
			}
		}
		Atom	&operator	[](uint16	i)	const{	return	code[index+i];	}
		Code	*getObject()				const{	return	object;	}
		uint16	getIndex()					const{	return	index;	}

		IPGMContext	operator	*()	const;
		void	dereference_once();
		
		bool	is_reference()	const{	return	data==REFERENCE;	}
		bool	is_undefined()	const{	return	data==UNDEFINED;	}

		void	patch_input_code(uint16	pgm_code_index,uint16	input_index)	const{	((InputLessPGMOverlay	*)overlay)->patch_input_code(pgm_code_index,input_index,0);	}

		uint16	addProduction(Code	*object,bool	check_for_existence)	const;	//	if check_for_existence==false, the object is assumed not to be new.

		template<class	C>	void	copy(C	*destination,uint16	write_index)	const{
			
			uint16	extent_index=0;
			copy_structure(destination,write_index,extent_index,true,-1);
		}

		template<class	C>	void	copy(C	*destination,uint16	write_index,uint16	&extent_index)	const{
			
			copy_structure(destination,write_index,extent_index,true,-1);
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
		bool	match(const	IPGMContext	&input)	const;

		//	called by operators.
		r_code::Code	*build_object(Atom	head)	const{	return	overlay->build_object(head);	}

		//	Implementation of executive-dependent operators.
		static	bool	Ins(const	IPGMContext	&context,uint16	&index);
		static	bool	Fvw(const	IPGMContext	&context,uint16	&index);
		static	bool	Red(const	IPGMContext	&context,uint16	&index);
	};
}


#endif