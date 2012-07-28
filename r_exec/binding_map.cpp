//	binding_map.cpp
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

#include	"mem.h"
#include	"binding_map.h"
#include	"factory.h"


namespace	r_exec{

	Value::Value(BindingMap	*map):_Object(),map(map){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	BoundValue::BoundValue(BindingMap	*map):Value(map){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	UnboundValue::UnboundValue(BindingMap	*map,uint8	index):Value(map),index(index){

		++map->unbound_values;
	}

	UnboundValue::~UnboundValue(){

		--map->unbound_values;
	}

	Value	*UnboundValue::copy(BindingMap	*map)	const{

		return	new	UnboundValue(map,index);
	}

	void	UnboundValue::valuate(Code	*destination,uint16	write_index,uint16	&extent_index)	const{

		destination->code(write_index)=Atom::VLPointer(index);
	}

	bool	UnboundValue::match(const	Code	*object,uint16	index){

		Atom	o_atom=object->code(index);
		switch(o_atom.getDescriptor()){
		case	Atom::I_PTR:
			map->bind_variable(new	StructureValue(map,object,o_atom.asIndex()),this->index);
			break;
		case	Atom::R_PTR:
			map->bind_variable(new	ObjectValue(map,object->get_reference(o_atom.asIndex())),this->index);
			break;
		case	Atom::WILDCARD:
			break;
		default:
			map->bind_variable(new	AtomValue(map,o_atom),this->index);
			break;
		}

		return	true;
	}

	Atom	*UnboundValue::get_code(){

		return	NULL;
	}

	Code	*UnboundValue::get_object(){

		return	NULL;
	}

	uint16	UnboundValue::get_code_size(){

		return	0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	AtomValue::AtomValue(BindingMap	*map,Atom	atom):BoundValue(map),atom(atom){
	}

	Value	*AtomValue::copy(BindingMap	*map)	const{

		return	new	AtomValue(map,atom);
	}

	void	AtomValue::valuate(Code	*destination,uint16	write_index,uint16	&extent_index)	const{

		destination->code(write_index)=atom;
	}

	bool	AtomValue::match(const	Code	*object,uint16	index){

		return	map->match_atom(object->code(index),atom);
	}

	Atom	*AtomValue::get_code(){

		return	&atom;
	}

	Code	*AtomValue::get_object(){

		return	NULL;
	}

	uint16	AtomValue::get_code_size(){

		return	1;
	}

	bool	AtomValue::intersect(const	Value	*v)	const{

		return	v->_intersect(this);
	}

	bool	AtomValue::_intersect(const	AtomValue	*v)	const{

		return	contains(v->atom);
	}

	bool	AtomValue::contains(const	Atom	a)	const{	
		
		if(atom==a)
			return	true;

		if(atom.isFloat()	&&	a.isFloat())
			return	Utils::Equal(atom.asFloat(),a.asFloat());

		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	StructureValue::StructureValue(BindingMap	*map,const	Code	*structure):BoundValue(map){

		this->structure=new	r_code::LObject();
		for(uint16	i=0;i<structure->code_size();++i)
			this->structure->code(i)=structure->code(i);
	}

	StructureValue::StructureValue(BindingMap	*map,const	Code	*source,uint16	structure_index):BoundValue(map){

		structure=new	r_code::LObject();
		for(uint16	i=0;i<=source->code(structure_index).getAtomCount();++i)
			structure->code(i)=source->code(structure_index+i);
	}

	StructureValue::StructureValue(BindingMap	*map,Atom	*source,uint16	structure_index):BoundValue(map){

		structure=new	r_code::LObject();
		for(uint16	i=0;i<=source[structure_index].getAtomCount();++i)
			structure->code(i)=source[structure_index+i];
	}

	StructureValue::StructureValue(BindingMap	*map,uint64	time):BoundValue(map){

		structure=new	r_code::LObject();
		structure->resize_code(3);
		Utils::SetTimestamp(&structure->code(0),time);
	}

	Value	*StructureValue::copy(BindingMap	*map)	const{

		return	new	StructureValue(map,structure);
	}

	void	StructureValue::valuate(Code	*destination,uint16	write_index,uint16	&extent_index)	const{

		destination->code(write_index)=Atom::IPointer(extent_index);
		for(uint16	i=0;i<=structure->code(0).getAtomCount();++i)
			destination->code(extent_index++)=structure->code(i);
	}

	bool	StructureValue::match(const	Code	*object,uint16	index){

		if(object->code(index).getDescriptor()!=Atom::I_PTR)
			return	false;
		return	map->match_structure(object,object->code(index).asIndex(),0,structure,0);
	}

	Atom	*StructureValue::get_code(){

		return	&structure->code(0);
	}

	Code	*StructureValue::get_object(){

		return	NULL;
	}

	uint16	StructureValue::get_code_size(){

		return	structure->code_size();
	}

	bool	StructureValue::intersect(const	Value	*v)	const{

		return	v->_intersect(this);
	}

	bool	StructureValue::_intersect(const	StructureValue	*v)	const{

		return	contains(&v->structure->code(0));
	}

	bool	StructureValue::contains(const	Atom	*s)	const{

		if(structure->code(0)!=s[0])
			return	false;
		if(structure->code(0).getDescriptor()==Atom::TIMESTAMP)
			return	Utils::Synchronous(Utils::GetTimestamp(&structure->code(0)),Utils::GetTimestamp(s));
		for(uint16	i=1;i<structure->code_size();++i){

			Atom	a=structure->code(i);
			Atom	_a=s[i];
			if(a==_a)
				continue;

			if(a.isFloat()	&&	_a.isFloat())
				return	Utils::Equal(a.asFloat(),_a.asFloat());

			return	false;
		}

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	ObjectValue::ObjectValue(BindingMap	*map,Code	*object):BoundValue(map),object(object){
	}

	Value	*ObjectValue::copy(BindingMap	*map)	const{

		return	new	ObjectValue(map,object);
	}

	void	ObjectValue::valuate(Code	*destination,uint16	write_index,uint16	&extent_index)	const{

		destination->code(write_index)=Atom::RPointer(destination->references_size());
		destination->add_reference(object);
	}

	bool	ObjectValue::match(const	Code	*object,uint16	index){

		return	map->match_object(object->get_reference(object->code(index).asIndex()),this->object);
	}

	Atom	*ObjectValue::get_code(){

		return	&object->code(0);
	}

	Code	*ObjectValue::get_object(){

		return	object;
	}

	uint16	ObjectValue::get_code_size(){

		return	object->code_size();
	}

	bool	ObjectValue::intersect(const	Value	*v)	const{

		return	v->_intersect(this);
	}

	bool	ObjectValue::_intersect(const	ObjectValue	*v)	const{

		return	contains(v->object);
	}

	bool	ObjectValue::contains(const	Code	*o)	const{
		
		return	object==o;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	_Fact	*BindingMap::abstract_f_ihlp(_Fact	*f_ihlp)	const{	// bindings are set already (coming from a mk.rdx caught by auto-focus).

		uint16	opcode;
		Code	*ihlp=f_ihlp->get_reference(0);
		if(ihlp->code(0).asOpcode()==Opcodes::ICst)
			opcode=Opcodes::ICst;
		else
			opcode=Opcodes::IMdl;

		_Fact	*_f_ihlp=new	Fact();
		for(uint16	i=0;i<f_ihlp->code_size();++i)
			_f_ihlp->code(i)=f_ihlp->code(i);

		Code	*_ihlp=_Mem::Get()->build_object(Atom::Object(opcode,I_HLP_ARITY));
		_ihlp->code(I_HLP_OBJ)=Atom::RPointer(0);

		uint16	extent_index=I_HLP_ARITY+1;

		uint32	map_index=0;

		uint16	tpl_arg_set_index=ihlp->code(I_HLP_TPL_ARGS).asIndex();
		uint16	tpl_arg_count=ihlp->code(tpl_arg_set_index).getAtomCount();
		
		_ihlp->code(I_HLP_TPL_ARGS)=Atom::IPointer(extent_index);
		_ihlp->code(extent_index)=Atom::Set(tpl_arg_count);
		for(uint16	i=1;i<=tpl_arg_count;++i)
			_ihlp->code(extent_index+i)=Atom::VLPointer(map_index++);
		extent_index+=tpl_arg_count;

		uint16	arg_set_index=ihlp->code(I_HLP_ARGS).asIndex();
		uint16	arg_count=ihlp->code(arg_set_index).getAtomCount();
		_ihlp->code(I_HLP_ARGS)=Atom::IPointer(extent_index);
		_ihlp->code(extent_index)=Atom::Set(arg_count);
		for(uint16	i=1;i<=arg_count;++i)
			_ihlp->code(extent_index+i)=Atom::VLPointer(map_index++);

		_ihlp->code(I_HLP_ARITY)=ihlp->code(I_HLP_ARITY);

		_ihlp->add_reference(f_ihlp->get_reference(f_ihlp->code(I_HLP_OBJ).asIndex()));

		_f_ihlp->add_reference(_ihlp);
		return	_f_ihlp;
	}

	_Fact	*BindingMap::abstract_fact(_Fact	*fact,_Fact	*original,bool	force_sync){	// abstract values as they are encountered.
		
		if(fwd_after_index==-1)
			first_index=map.size();

		uint16	extent_index=FACT_ARITY+1;
		abstract_member(original,FACT_OBJ,fact,FACT_OBJ,extent_index);
		if(fwd_after_index!=-1	&&	force_sync){
			
			fact->code(FACT_AFTER)=Atom::VLPointer(fwd_after_index);
			fact->code(FACT_BEFORE)=Atom::VLPointer(fwd_before_index);
		}else{
			
			abstract_member(original,FACT_AFTER,fact,FACT_AFTER,extent_index);
			abstract_member(original,FACT_BEFORE,fact,FACT_BEFORE,extent_index);
		}
		fact->code(FACT_CFD)=Atom::Wildcard();
		fact->code(FACT_ARITY)=Atom::Wildcard();

		if(fwd_after_index==-1){

			fwd_after_index=map.size()-2;
			fwd_before_index=fwd_after_index+1;
		}

		return	fact;
	}

	Code	*BindingMap::abstract_object(Code	*object,bool	force_sync){	// abstract values as they are encountered.
		
		Code	*abstracted_object=NULL;

		uint16	opcode=object->code(0).asOpcode();
		if(opcode==Opcodes::Fact)
			return	abstract_fact(new	Fact(),(_Fact	*)object,force_sync);
		else	if(opcode==Opcodes::AntiFact)
			return	abstract_fact(new	AntiFact(),(_Fact	*)object,force_sync);
		else	if(opcode==Opcodes::Cmd){

			uint16	extent_index=CMD_ARITY+1;
			abstracted_object=_Mem::Get()->build_object(object->code(0));
			abstracted_object->code(CMD_FUNCTION)=object->code(CMD_FUNCTION);
			abstract_member(object,CMD_ARGS,abstracted_object,CMD_ARGS,extent_index);
			abstracted_object->code(CMD_ARITY)=Atom::Wildcard();
		}else	if(opcode==Opcodes::MkVal){

			uint16	extent_index=MK_VAL_ARITY+1;
			abstracted_object=_Mem::Get()->build_object(object->code(0));
			abstract_member(object,MK_VAL_OBJ,abstracted_object,MK_VAL_OBJ,extent_index);
			abstract_member(object,MK_VAL_ATTR,abstracted_object,MK_VAL_ATTR,extent_index);
			abstract_member(object,MK_VAL_VALUE,abstracted_object,MK_VAL_VALUE,extent_index);
			abstracted_object->code(MK_VAL_ARITY)=Atom::Wildcard();
		}else	if(opcode==Opcodes::IMdl	||	opcode==Opcodes::ICst){

			uint16	extent_index=I_HLP_ARITY+1;
			abstracted_object=_Mem::Get()->build_object(object->code(0));
			abstract_member(object,I_HLP_OBJ,abstracted_object,I_HLP_OBJ,extent_index);
			abstract_member(object,I_HLP_TPL_ARGS,abstracted_object,I_HLP_TPL_ARGS,extent_index);
			abstract_member(object,I_HLP_ARGS,abstracted_object,I_HLP_ARGS,extent_index);
			abstracted_object->code(I_HLP_WR_E)=Atom::Wildcard();
			abstracted_object->code(I_HLP_ARITY)=Atom::Wildcard();
		}else
			return	object;
		return	abstracted_object;
	}

	void	BindingMap::abstract_member(Code	*object,uint16	index,Code	*abstracted_object,uint16	write_index,uint16	&extent_index){

		Atom	a=object->code(index);
		uint16	ai=a.asIndex();
		switch(a.getDescriptor()){
		case	Atom::R_PTR:{
			Code	*reference=object->get_reference(ai);
			if(reference->code(0).asOpcode()==Opcodes::Ont){	// ontologies resist abstraction.

				abstracted_object->code(write_index)=Atom::RPointer(abstracted_object->references_size());
				abstracted_object->add_reference(reference);
			}else	if(reference->code(0).asOpcode()==Opcodes::Ent)	// entities are always abstracted.
				abstracted_object->code(write_index)=get_object_variable(reference);
			else{	// abstract the reference.

				abstracted_object->code(write_index)=Atom::RPointer(abstracted_object->references_size());
				abstracted_object->add_reference(abstract_object(reference,false));
			}
			break;
		}case	Atom::I_PTR:
			if(object->code(ai).getDescriptor()==Atom::SET){

				abstracted_object->code(write_index)=Atom::IPointer(extent_index);

				uint16	element_count=object->code(ai).getAtomCount();
				abstracted_object->code(extent_index)=Atom::Set(element_count);
				uint16	_write_index=extent_index;
				extent_index+=element_count+1;
				for(uint16	i=1;i<=element_count;++i)
					abstract_member(object,ai+i,abstracted_object,_write_index+i,extent_index);
			}else
				abstracted_object->code(write_index)=get_structure_variable(object,ai);
			break;
		default:
			abstracted_object->code(write_index)=get_atom_variable(a);
			break;
		}
	}

	void	BindingMap::init(Code	*object,uint16	index){

		Atom	a=object->code(index);
		switch(a.getDescriptor()){
		case	Atom::R_PTR:
			get_object_variable(object->get_reference(a.asIndex()));
			break;
		case	Atom::I_PTR:
			get_structure_variable(object,a.asIndex());
			break;
		default:
			get_atom_variable(a);
			break;
		}
	}

	Atom	BindingMap::get_atom_variable(Atom	a){

		for(uint32	i=0;i<map.size();++i){

			if(map[i]->contains(a))
				return	Atom::VLPointer(i);
		}

		uint32	size=map.size();
		map.push_back(new	AtomValue(this,a));
		return	Atom::VLPointer(size);
	}

	Atom	BindingMap::get_structure_variable(Code	*object,uint16	index){

		for(uint32	i=0;i<map.size();++i){

			if(map[i]->contains(&object->code(index)))
				return	Atom::VLPointer(i);
		}

		uint32	size=map.size();
		map.push_back(new	StructureValue(this,object,index));
		return	Atom::VLPointer(size);
	}

	Atom	BindingMap::get_object_variable(Code	*object){

		for(uint32	i=0;i<map.size();++i){

			if(map[i]->contains(object))
				return	Atom::VLPointer(i);
		}

		uint32	size=map.size();
		map.push_back(new	ObjectValue(this,object));
		return	Atom::VLPointer(size);
	}

	BindingMap::BindingMap():_Object(),fwd_after_index(-1),fwd_before_index(-1),unbound_values(0){
	}

	BindingMap::BindingMap(const	BindingMap	*source):_Object(){

		*this=*source;
	}

	BindingMap::BindingMap(const	BindingMap	&source):_Object(){

		*this=source;
	}

	BindingMap::~BindingMap(){
	}

	void	BindingMap::clear(){

		map.clear();
		fwd_after_index=fwd_before_index=-1;
	}

	BindingMap	&BindingMap::operator	=(const	BindingMap	&source){

		clear();
		for(uint8	i=0;i<source.map.size();++i)
			map.push_back(source.map[i]->copy(this));
		first_index=source.first_index;
		fwd_after_index=source.fwd_after_index;
		fwd_before_index=source.fwd_before_index;
		unbound_values=source.unbound_values;
		return	*this;
	}

	void	BindingMap::load(const	BindingMap	*source){

		*this=*source;
	}

	void	BindingMap::add_unbound_value(uint8	id){

		if(id>=map.size())
			map.resize(id+1);
		map[id]=new	UnboundValue(this,id);
	}

	bool	BindingMap::match(const	Code	*object,uint16	o_base_index,uint16	o_index,const	Code	*pattern,uint16	p_index,uint16	o_arity){

		uint16	o_full_index=o_base_index+o_index;
		Atom	o_atom=object->code(o_full_index);
		Atom	p_atom=pattern->code(p_index);
		switch(o_atom.getDescriptor()){
		case	Atom::T_WILDCARD:
		case	Atom::WILDCARD:
		case	Atom::VL_PTR:
			break;
		case	Atom::I_PTR:
			switch(p_atom.getDescriptor()){
			case	Atom::VL_PTR:
				if(!map[p_atom.asIndex()]->match(object,o_full_index))
					return	false;
				break;
			case	Atom::I_PTR:
				if(!match_structure(object,o_atom.asIndex(),0,pattern,p_atom.asIndex()))
					return	false;
				break;
			case	Atom::T_WILDCARD:
			case	Atom::WILDCARD:
				break;
			default:
				return	false;
			}
			break;
		case	Atom::R_PTR:
			switch(p_atom.getDescriptor()){
			case	Atom::VL_PTR:
				if(!map[p_atom.asIndex()]->match(object,o_full_index))
					return	false;
				break;
			case	Atom::R_PTR:
				if(!match_object(object->get_reference(o_atom.asIndex()),pattern->get_reference(p_atom.asIndex())))
					return	false;
				break;
			case	Atom::T_WILDCARD:
			case	Atom::WILDCARD:
				break;
			default:
				return	false;
			}
			break;
		default:
			switch(p_atom.getDescriptor()){
			case	Atom::VL_PTR:
				if(!map[p_atom.asIndex()]->match(object,o_full_index))
					return	false;
				break;
			case	Atom::T_WILDCARD:
			case	Atom::WILDCARD:
				break;
			default:
				if(!match_atom(o_atom,p_atom))
					return	false;
				break;
			}
			break;
		}

		if(o_index==o_arity)
			return	true;
		return	match(object,o_base_index,o_index+1,pattern,p_index+1,o_arity);
	}

	bool	BindingMap::match_atom(Atom	o_atom,Atom	p_atom){

		if(p_atom==o_atom)
			return	true;

		if(p_atom.isFloat()	&&	o_atom.isFloat())
			return	Utils::Equal(o_atom.asFloat(),p_atom.asFloat());

		return	false;
	}

	bool	BindingMap::match_structure(const	Code	*object,uint16	o_base_index,uint16	o_index,const	Code	*pattern,uint16	p_index){

		uint16	o_full_index=o_base_index+o_index;
		Atom	o_atom=object->code(o_full_index);
		Atom	p_atom=pattern->code(p_index);
		if(o_atom!=p_atom)
			return	false;
		uint16	arity=o_atom.getAtomCount();
		if(arity==0)	// empty sets.
			return	true;
		if(o_atom.getDescriptor()==Atom::TIMESTAMP)
			return	Utils::Synchronous(Utils::GetTimestamp(&object->code(o_full_index)),Utils::GetTimestamp(&pattern->code(p_index)));
		return	match(object,o_base_index,o_index+1,pattern,p_index+1,arity);
	}

	void	BindingMap::reset_fwd_timings(_Fact	*reference_fact){	// valuate at after_index and after_index+1 from the timings of the reference object.

		map[fwd_after_index]=new	StructureValue(this,reference_fact,reference_fact->code(FACT_AFTER).asIndex());
		map[fwd_before_index]=new	StructureValue(this,reference_fact,reference_fact->code(FACT_BEFORE).asIndex());
	}

	bool	BindingMap::match_timings(uint64	stored_after,uint64	stored_before,uint64	after,uint64	before,uint32	destination_after_index,uint32	destination_before_index){

		if(stored_after<=after){

			if(stored_before>=before){			// sa a b sb

				Utils::SetTimestamp(map[destination_after_index]->get_code(),after);
				Utils::SetTimestamp(map[destination_before_index]->get_code(),before);
				return	true;
			}else{

				if(stored_before>after){		// sa a sb b

					Utils::SetTimestamp(map[destination_after_index]->get_code(),after);
					return	true;
				}
				return	false;
			}
		}else{

			if(stored_before<=before)			// a sa sb b
				return	true;
			else	if(stored_after<before){	// a sa b sb

				Utils::SetTimestamp(map[destination_before_index]->get_code(),before);
				return	true;
			}
			return	false;
		}
	}

	bool	BindingMap::match_fwd_timings(const	_Fact	*f_object,const	_Fact	*f_pattern){

		return	match_timings(get_fwd_after(),get_fwd_before(),f_object->get_after(),f_object->get_before(),fwd_after_index,fwd_before_index);
	}

	bool	BindingMap::match_fwd_strict(const	_Fact	*f_object,const	_Fact	*f_pattern){

		if(match_object(f_object->get_reference(0),f_pattern->get_reference(0))){

			if(f_object->code(0)!=f_pattern->code(0))
				return	false;

			return	match_fwd_timings(f_object,f_pattern);
		}else
			return	false;
	}

	MatchResult	BindingMap::match_fwd_lenient(const	_Fact	*f_object,const	_Fact	*f_pattern){

		if(match_object(f_object->get_reference(0),f_pattern->get_reference(0))){

			MatchResult	r;
			if(f_pattern->code(0)==f_object->code(0))
				r=MATCH_SUCCESS_POSITIVE;
			else
				r=MATCH_SUCCESS_NEGATIVE;

			if(match_fwd_timings(f_object,f_pattern))
				return	r;
			return	MATCH_FAILURE;
		}else
			return	MATCH_FAILURE;
	}

	uint64	BindingMap::get_fwd_after()		const{

		return	Utils::GetTimestamp(map[fwd_after_index]->get_code());
	}

	uint64	BindingMap::get_fwd_before()	const{

		return	Utils::GetTimestamp(map[fwd_before_index]->get_code());
	}

	bool	BindingMap::match_object(const	Code	*object,const	Code	*pattern){

		if(object->code(0)!=pattern->code(0))
			return	false;
		uint16	pattern_opcode=pattern->code(0).asOpcode();
		if(	pattern_opcode==Opcodes::Ent	||
			pattern_opcode==Opcodes::Ont	||
			pattern_opcode==Opcodes::Mdl	||
			pattern_opcode==Opcodes::Cst)
			return	object==pattern;
		return	match(object,0,1,pattern,1,object->code(0).getAtomCount());
	}

	void	BindingMap::bind_variable(BoundValue	*value,uint8	id){

		map[id]=value;
	}

	void	BindingMap::bind_variable(Atom	*code,uint8	id,uint16	value_index,Atom	*intermediate_results){	// assigment.

		Atom	v_atom=code[value_index];
		if(v_atom.isFloat())
			bind_variable(new	AtomValue(this,v_atom),id);
		else	switch(v_atom.getDescriptor()){
		case	Atom::VALUE_PTR:	
			bind_variable(new	StructureValue(this,intermediate_results,v_atom.asIndex()),id);
			break;
		}
	}

	Atom	*BindingMap::get_value_code(uint16	id){

		return	map[id]->get_code();
	}

	uint16	BindingMap::get_value_code_size(uint16	id){

		return	map[id]->get_code_size();
	}

	bool	BindingMap::scan_variable(uint16	id)	const{

		if(id<first_index)
			return	true;
		return	(map[id]->get_code()!=NULL);
	}

	bool	BindingMap::intersect(BindingMap	*bm){

		for(uint32	i=0;i<map.size();){

			if(i==fwd_after_index){	// ignore fact timings.

				i+=2;
				continue;
			}

			for(uint32	j=0;j<bm->map.size();){

				if(j==bm->fwd_after_index){	// ignore fact timings.

					j+=2;
					continue;
				}

				if(map[i]->intersect(bm->map[j]))
					return	true;

				++j;
			}

			++i;
		}

		return	false;
	}

	bool	BindingMap::is_fully_specified()	const{

		return	unbound_values==0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	HLPBindingMap::HLPBindingMap():BindingMap(),bwd_after_index(-1),bwd_before_index(-1){
	}

	HLPBindingMap::HLPBindingMap(const	HLPBindingMap	*source):BindingMap(){

		*this=*source;
	}

	HLPBindingMap::HLPBindingMap(const	HLPBindingMap	&source):BindingMap(){

		*this=source;
	}

	HLPBindingMap::~HLPBindingMap(){
	}

	void	HLPBindingMap::clear(){

		BindingMap::clear();
		bwd_after_index=bwd_before_index=-1;
	}

	HLPBindingMap	&HLPBindingMap::operator	=(const	HLPBindingMap	&source){

		clear();
		for(uint8	i=0;i<source.map.size();++i)
			map.push_back(source.map[i]->copy(this));
		first_index=source.first_index;
		fwd_after_index=source.fwd_after_index;
		fwd_before_index=source.fwd_before_index;
		bwd_after_index=source.bwd_after_index;
		bwd_before_index=source.bwd_before_index;
		unbound_values=source.unbound_values;
		return	*this;
	}

	void	HLPBindingMap::load(const	HLPBindingMap	*source){

		*this=*source;
	}

	void	HLPBindingMap::init_from_pattern(const	Code	*source,int16	position){	// source is abstracted.

		bool	set_fwd_timing_index=(position==0);
		bool	set_bwd_timing_index=(position==1);
		for(uint16	i=1;i<source->code_size();++i){

			Atom	s=source->code(i);
			switch(s.getDescriptor()){
			case	Atom::VL_PTR:{
				uint8	value_index=source->code(i).asIndex();
				add_unbound_value(value_index);
				if(set_fwd_timing_index	&&	i==FACT_AFTER)
					fwd_after_index=value_index;
				else	if(set_fwd_timing_index	&&	i==FACT_BEFORE){

					fwd_before_index=value_index;
					set_fwd_timing_index=false;
				}else	if(set_bwd_timing_index	&&	i==FACT_AFTER)
					bwd_after_index=value_index;
				else	if(set_bwd_timing_index	&&	i==FACT_BEFORE){

					bwd_before_index=value_index;
					set_bwd_timing_index=false;
				}
				break;
			}default:
				break;
			}
		}

		for(uint16	i=0;i<source->references_size();++i)
			init_from_pattern(source->get_reference(i),-1);
	}

	void	HLPBindingMap::init_from_hlp(const	Code	*hlp){	// hlp is cst or mdl.

		uint16	tpl_arg_set_index=hlp->code(HLP_TPL_ARGS).asIndex();
		uint16	tpl_arg_count=hlp->code(tpl_arg_set_index).getAtomCount();
		for(uint16	i=1;i<=tpl_arg_count;++i){

			Atom	a=hlp->code(tpl_arg_set_index+i);
			if(a.getDescriptor()==Atom::VL_PTR)
				add_unbound_value(a.asIndex());
		}

		first_index=map.size();
		
		uint16	obj_set_index=hlp->code(HLP_OBJS).asIndex();
		uint16	obj_count=hlp->code(obj_set_index).getAtomCount();
		for(uint16	i=1;i<=obj_count;++i){

			_Fact	*pattern=(_Fact	*)hlp->get_reference(hlp->code(obj_set_index+i).asIndex());
			init_from_pattern(pattern,i-1);
		}
	}

	void	HLPBindingMap::init_from_f_ihlp(const	_Fact	*f_ihlp){	// source is f->icst or f->imdl; map already initialized with values from hlp.

		Code	*ihlp=f_ihlp->get_reference(0);

		uint16	tpl_val_set_index=ihlp->code(I_HLP_TPL_ARGS).asIndex();
		uint16	tpl_val_count=ihlp->code(tpl_val_set_index++).getAtomCount();
		for(uint16	i=0;i<tpl_val_count;++i){	// valuate tpl args.

			Atom	atom=ihlp->code(tpl_val_set_index+i);
			switch(atom.getDescriptor()){
			case	Atom::R_PTR:
				map[i]=new	ObjectValue(this,ihlp->get_reference(atom.asIndex()));
				break;
			case	Atom::I_PTR:
				map[i]=new	StructureValue(this,ihlp,atom.asIndex());
				break;
			default:
				map[i]=new	AtomValue(this,atom);
				break;
			}
		}

		uint16	val_set_index=ihlp->code(I_HLP_ARGS).asIndex()+1;
		uint32	i=0;
		for(uint32	j=first_index;j<map.size();++j){	// valuate args.

			if(j==fwd_after_index	||	j==fwd_before_index)
				continue;

			Atom	atom=ihlp->code(val_set_index+i);
			switch(atom.getDescriptor()){
			case	Atom::R_PTR:
				map[j]=new	ObjectValue(this,ihlp->get_reference(atom.asIndex()));
				break;
			case	Atom::I_PTR:
				map[j]=new	StructureValue(this,ihlp,atom.asIndex());
				break;
			case	Atom::WILDCARD:
			case	Atom::T_WILDCARD:
			case	Atom::VL_PTR:
				break;
			default:
				map[j]=new	AtomValue(this,atom);
				break;
			}
		}

		map[fwd_after_index]=new	StructureValue(this,f_ihlp,FACT_AFTER);	// valuate timings; fwd_after_index is already known.
		map[fwd_before_index]=new	StructureValue(this,f_ihlp,FACT_BEFORE);
	}

	Fact	*HLPBindingMap::build_f_ihlp(Code	*hlp,uint16	opcode,bool	wr_enabled)	const{

		Code	*ihlp=_Mem::Get()->build_object(Atom::Object(opcode,I_HLP_ARITY));
		ihlp->code(I_HLP_OBJ)=Atom::RPointer(0);
		ihlp->add_reference(hlp);

		uint16	tpl_arg_index=I_HLP_ARITY+1;
		ihlp->code(I_HLP_TPL_ARGS)=Atom::IPointer(tpl_arg_index);
		ihlp->code(tpl_arg_index)=Atom::Set(first_index);
		uint16	write_index=tpl_arg_index+1;
		uint16	extent_index=write_index+first_index;
		for(uint16	i=0;i<first_index;++i){	// valuate tpl args.

			map[i]->valuate(ihlp,write_index,extent_index);
			++write_index;
		}

		ihlp->code(I_HLP_ARGS)=Atom::IPointer(extent_index);
		uint16	exposed_arg_start=first_index;
		uint16	exposed_arg_count=map.size()-exposed_arg_start-2;	// -2: do not expose the first after/before timestamps.
		ihlp->code(extent_index)=Atom::Set(exposed_arg_count);

		write_index=extent_index+1;
		extent_index=write_index+exposed_arg_count;
		for(uint16	i=exposed_arg_start;i<map.size();++i){	// valuate args.

			if(i==fwd_after_index)
				continue;
			if(i==fwd_before_index)
				continue;
			map[i]->valuate(ihlp,write_index,extent_index);
			++write_index;
		}

		ihlp->code(I_HLP_WR_E)=Atom::Boolean(wr_enabled);
		ihlp->code(I_HLP_ARITY)=Atom::Float(1);	// psln_thr.

		Fact	*f_ihlp=new	Fact(ihlp,0,0,1,1);
		extent_index=FACT_ARITY+1;
		map[fwd_after_index]->valuate(f_ihlp,FACT_AFTER,extent_index);
		map[fwd_before_index]->valuate(f_ihlp,FACT_BEFORE,extent_index);
		return	f_ihlp;
	}

	Code	*HLPBindingMap::bind_pattern(Code	*pattern)	const{

		if(!need_binding(pattern))
			return	pattern;

		Code	*bound_pattern=_Mem::Get()->build_object(pattern->code(0));

		for(uint16	i=0;i<pattern->references_size();++i){	// bind references when needed; must be done before binding the code as this may add references.

			Code	*reference=pattern->get_reference(i);
			if(need_binding(reference))
				bound_pattern->set_reference(i,bind_pattern(reference));
			else
				bound_pattern->set_reference(i,reference);
		}

		uint16	extent_index=pattern->code_size();
		for(uint16	i=1;i<pattern->code_size();++i){	// transform code containing variables into actual values when they exist: objects -> r_ptr, structures -> i_ptr, atoms -> atoms.
			
			Atom	p_atom=pattern->code(i);
			switch(p_atom.getDescriptor()){
			case	Atom::VL_PTR:
				map[p_atom.asIndex()]->valuate(bound_pattern,i,extent_index);
				break;
			case	Atom::TIMESTAMP:
			case	Atom::STRING:{	// avoid misinterpreting raw data that could be lead by descriptors.
				bound_pattern->code(i)=p_atom;
				uint16	atom_count=p_atom.getAtomCount();
				for(uint16	j=i+1;j<=i+atom_count;++j)
					bound_pattern->code(j)=pattern->code(j);
				i+=atom_count;
				break;
			}default:
				bound_pattern->code(i)=p_atom;
				break;
			}
		}

		return	bound_pattern;
	}

	bool	HLPBindingMap::need_binding(Code	*pattern)	const{

		if(	pattern->code(0).asOpcode()==Opcodes::Ont	||
			pattern->code(0).asOpcode()==Opcodes::Ent	||
			pattern->code(0).asOpcode()==Opcodes::Mdl	||
			pattern->code(0).asOpcode()==Opcodes::Cst)
			return	false;
		
		for(uint16	i=0;i<pattern->references_size();++i){

			if(need_binding(pattern->get_reference(i)))
				return	true;
		}

		for(uint16	i=1;i<pattern->code_size();++i){
			
			Atom	p_atom=pattern->code(i);
			switch(p_atom.getDescriptor()){
			case	Atom::VL_PTR:
				return	true;
			case	Atom::TIMESTAMP:
			case	Atom::STRING:
				i+=p_atom.getAtomCount();
				break;
			default:
				break;
			}
		}

		return	false;
	}

	void	HLPBindingMap::reset_bwd_timings(_Fact	*reference_fact){	// valuate at after_index and after_index+1 from the timings of the reference fact.

		map[bwd_after_index]=new	StructureValue(this,reference_fact,reference_fact->code(FACT_AFTER).asIndex());
		map[bwd_before_index]=new	StructureValue(this,reference_fact,reference_fact->code(FACT_BEFORE).asIndex());
	}

	bool	HLPBindingMap::match_bwd_timings(const	_Fact	*f_object,const	_Fact	*f_pattern){

		return	match_timings(get_bwd_after(),get_bwd_before(),f_object->get_after(),f_object->get_before(),bwd_after_index,bwd_before_index);
	}

	bool	HLPBindingMap::match_bwd_strict(const	_Fact	*f_object,const	_Fact	*f_pattern){

		if(match_object(f_object->get_reference(0),f_pattern->get_reference(0))){

			if(f_object->code(0)!=f_pattern->code(0))
				return	false;

			return	match_bwd_timings(f_object,f_pattern);
		}else
			return	false;
	}

	MatchResult	HLPBindingMap::match_bwd_lenient(const	_Fact	*f_object,const	_Fact	*f_pattern){

		if(match_object(f_object->get_reference(0),f_pattern->get_reference(0))){

			MatchResult	r;
			if(f_pattern->code(0)==f_object->code(0))
				r=MATCH_SUCCESS_POSITIVE;
			else
				r=MATCH_SUCCESS_NEGATIVE;

			if(match_bwd_timings(f_object,f_pattern))
				return	r;
			return	MATCH_FAILURE;
		}else
			return	MATCH_FAILURE;
	}

	uint64	HLPBindingMap::get_bwd_after()		const{

		return	Utils::GetTimestamp(map[bwd_after_index]->get_code());
	}

	uint64	HLPBindingMap::get_bwd_before()	const{

		return	Utils::GetTimestamp(map[bwd_before_index]->get_code());
	}
}