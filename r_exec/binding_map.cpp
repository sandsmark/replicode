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


namespace	r_exec{

	BindingMap::BindingMap():_Object(){
	}

	BindingMap::BindingMap(const	BindingMap	*source):_Object(){

		init(source);
	}

	void	BindingMap::clear(){

		objects.clear();
		atoms.clear();
		structures.clear();
	}

	void	BindingMap::init(const	BindingMap	*source){

		atoms=source->atoms;
		structures=source->structures;
		objects=source->objects;
	}

	void	BindingMap::init(Code	*source){	//	source is abstracted.

		if(source->code(0).asOpcode()==Opcodes::Var)
			objects[source]=source;
		else{
		
			for(uint16	i=0;i<source->code_size();++i){

				switch(source->code(i).getDescriptor()){
				case	Atom::NUMERICAL_VARIABLE:
				case	Atom::BOOLEAN_VARIABLE:
					atoms[source->code(i)]=source->code(i);
					break;
				case	Atom::STRUCTURAL_VARIABLE:{
					std::vector<Atom>	v;
					for(uint16	j=0;j<=source->code(i-1).getAtomCount();++j)
						v.push_back(source->code(i-1+j));
					structures[source->code(i)]=v;
					break;
				}
				}
			}

			for(uint16	i=0;i<source->references_size();++i)
				init(source->get_reference(i));
		}
	}

	Code	*BindingMap::bind_object(Code	*original)	const{

		if(original->code(0).asOpcode()==Opcodes::Var){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	o=objects.find(original);
			if(o!=objects.end()	&&	o->second!=NULL)
				return	o->second;
			else
				return	original;	//	no registered value; original is left unbound.
		}

		Code	*bound_object=_Mem::Get()->build_object(original->code(0));
		for(uint16	i=0;i<original->code_size();){	//	patch code containing variables with actual values when they exist.
			
			Atom	a=original->code(i);
			switch(a.getDescriptor()){
			case	Atom::NUMERICAL_VARIABLE:
			case	Atom::BOOLEAN_VARIABLE:{
				
				UNORDERED_MAP<Atom,Atom>::const_iterator	_a=atoms.find(a);
				if(_a==atoms.end())	//	no registered value; original is left unbound.
					bound_object->code(i++)=a;
				else
					bound_object->code(i++)=_a->second;
				break;
			}case	Atom::STRUCTURAL_VARIABLE:{

				UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	_s=structures.find(a);
				if(_s==structures.end())	//	no registered value; original is left unbound.
					bound_object->code(i++)=a;
				else{
					
					if(bound_object->code(i-1).getDescriptor()==Atom::TIMESTAMP)	//	store the tolerance in the timestamp (used to create monitoring jobs for goals).
						bound_object->code(i-1).setTolerance(a.getTolerance());
					for(uint16	j=1;j<_s->second.size();++j)
						bound_object->code(i++)=_s->second[j];
				}
				break;
			}default:
				bound_object->code(i++)=a;
				break;
			}
		}
		
		for(uint16	i=0;i<original->references_size();++i)	//	bind references when needed.
			if(needs_binding(original->get_reference(i)))
				bound_object->set_reference(i,bind_object(original->get_reference(i)));
			else
				bound_object->set_reference(i,original->get_reference(i));

		return	bound_object;
	}

	bool	BindingMap::needs_binding(Code	*original)	const{

		if(original->code(0).asOpcode()==Opcodes::Var){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b=objects.find(original);
			if(b!=objects.end())
				return	true;
			else
				return	false;
		}

		for(uint16	i=0;i<original->code_size();++i){		//	find code containing variables.

			switch(original->code(i).getDescriptor()){
			case	Atom::NUMERICAL_VARIABLE:
			case	Atom::STRUCTURAL_VARIABLE:
				return	true;
			}
		}

		for(uint16	i=0;i<original->references_size();++i){

			if(needs_binding(original->get_reference(i)))
				return	true;
		}

		return	false;
	}

	Atom	BindingMap::get_atomic_variable(const	Code	*object,uint16	index){

		Atom	var;
		Atom	val=object->code(index);

		if(val.isFloat()){

			float32	tolerance=_Mem::Get()->get_float_tolerance();
			float32	min=val.asFloat()*(1-tolerance);
			float32	max=val.asFloat()*(1+tolerance);

			UNORDERED_MAP<Atom,Atom>::const_iterator	a;
			for(a=atoms.begin();a!=atoms.end();++a)
				if(a->second.asFloat()>=min	&&
					a->second.asFloat()<=max){	//	variable already exists for the value.

					var=a->first;
					break;
				}
			
			if(!var){

				var=Atom::NumericalVariable(atoms.size(),Atom::GetTolerance(tolerance));
				atoms[var]=val;
			}
		}else	if(val.getDescriptor()==Atom::BOOLEAN_){

			UNORDERED_MAP<Atom,Atom>::const_iterator	a;
			for(a=atoms.begin();a!=atoms.end();++a)
				if(a->second.asBoolean()==val.asBoolean()){	//	variable already exists for the value.

					var=a->first;
					break;
				}

			if(!var){

				var=Atom::BooleanVariable(atoms.size());
				atoms[var]=val;
			}
		}

		return	var;
	}

	Atom	BindingMap::get_structural_variable(const	Code	*object,uint16	index){

		Atom	var;
		Atom	*val=&object->code(index);

		switch(val[0].getDescriptor()){
		case	Atom::TIMESTAMP:{	//	tolerance is used.
			
			float32	tolerance=_Mem::Get()->get_time_tolerance();
			uint64	t=Utils::GetTimestamp(val);
			uint64	min;
			uint64	max;
			uint64	now;
			if(t>0){

				if(t>65535)
					now=Now();
				else
					now=0;
				int64	delta_t=now-t;
				if(delta_t<0)
					delta_t=-delta_t;
				min=delta_t*(1-tolerance);
				max=delta_t*(1+tolerance);
			}else
				min=max=0;

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
			for(s=structures.begin();s!=structures.end();++s){

				if(val[0]==s->second[0]){

					int64	_t=Utils::GetTimestamp(&s->second[0]);
					if(t>0)
						_t-=now;
					if(_t<0)
						_t=-_t;
					if(_t>=min	&&	_t<=max){	//	variable already exists for the value.

						var=s->first;
						break;
					}
				}
			}

			if(!var){

				var=Atom::StructuralVariable(structures.size(),Atom::GetTolerance(tolerance));
				std::vector<Atom>	_value;
				for(uint16	i=0;i<=val[0].getAtomCount();++i)
					_value.push_back(val[i]);
				structures[var]=_value;
			}
			break;
		}case	Atom::STRING:{	//	tolerance is not used.

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
			for(s=structures.begin();s!=structures.end();++s){

				if(val[0]==s->second[0]){

					uint16	i;
					for(i=1;i<=val[0].getAtomCount();++i)
						if(val[i]!=s->second[i])
							break;
					if(i==val[0].getAtomCount()+1){

						var=s->first;
						break;
					}
				}
			}

			if(!var){

				var=Atom::StructuralVariable(structures.size(),Atom::GetTolerance(0));
				std::vector<Atom>	_value;
				for(uint16	i=0;i<=val[0].getAtomCount();++i)
					_value.push_back(val[i]);
				structures[var]=_value;
			}
			break;
		}case	Atom::OBJECT:{	//	tolerance is used to compare numerical structure members.

			float32	tolerance=_Mem::Get()->get_float_tolerance();

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
			for(s=structures.begin();s!=structures.end();++s){

				if(val[0]==s->second[0]){

					uint16	i;
					for(i=1;i<=val[0].getAtomCount();++i){

						if(val[i].isFloat()){

							float32	min=val[i].asFloat()*(1-tolerance);
							float32	max=val[i].asFloat()*(1+tolerance);
							if(s->second[i].asFloat()<min	||	s->second[i].asFloat()>max)
								break;
						}else	if(val[i]!=s->second[i])
							break;
					}
					if(i==val[0].getAtomCount()+1){

						var=s->first;
						break;
					}
				}
			}

			if(!var){

				var=Atom::StructuralVariable(structures.size(),Atom::GetTolerance(tolerance));
				std::vector<Atom>	_value;
				for(uint16	i=0;i<=val[0].getAtomCount();++i)
					_value.push_back(val[i]);
				structures[var]=_value;
			}
			break;
		}
		}

		return	var;
	}

	Code	*BindingMap::get_variable_object(Code	*object){

		Code	*var=NULL;
		bool	exists=false;

		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	o;
		for(o=objects.begin();o!=objects.end();++o)
			if(o->second==object){	//	variable already exists for the value.

				var=o->first;
				break;
			}
		
		if(!var){

			var=factory::Object::Var(1);
			P<Code>	p=object;
			objects.insert(std::pair<Code	*,P<Code> >(var,p));
		}

		return	var;
	}

	bool	BindingMap::match(Code	*object,Code	*pattern){

		if(pattern->code(0).asOpcode()==Opcodes::Var){

			if(!bind_object_variable(object,pattern))
				return	false;
			return	true;
		}else{

			if(object->code(0)!=pattern->code(0))
				return	false;
			if(object->code_size()!=pattern->code_size())
				return	false;
			if(object->references_size()!=pattern->references_size())
				return	false;

			for(uint16	i=0;i<pattern->code_size();){

				Atom	a=object->code(i);
				Atom	p=pattern->code(i);
				if(a!=p){

					switch(p.getDescriptor()){
					case	Atom::NUMERICAL_VARIABLE:
						if(!a.isFloat())
							return	false;
						if(!bind_float_variable(a,p))
							return	false;
						++i;
						break;
					case	Atom::BOOLEAN_VARIABLE:
						if(a.getDescriptor()!=Atom::BOOLEAN_)
							return	false;
						if(!bind_boolean_variable(a,p))
							return	false;
						++i;
						break;
					case	Atom::STRUCTURAL_VARIABLE:	//	necessarily of the same type (otherwise would have returned false at i-1).
						if(!bind_structural_variable(&object->code(i-1),p))
							return	false;
						i+=pattern->code(i-1).getAtomCount();	//	skip the rest of the structure.
						break;
					default:
						return	false;
					}
				}else
					++i;
			}

			for(uint16	i=0;i<pattern->references_size();++i){

				Code	*object_reference=object->get_reference(i);
				Code	*pattern_reference=pattern->get_reference(i);
				if(pattern_reference!=object_reference){

					if(!match(object_reference,pattern_reference))
						return	false;
				}
			}

			return	true;
		}
	}

	bool	BindingMap::bind_float_variable(Atom	val,Atom	var){

		UNORDERED_MAP<Atom,Atom>::const_iterator	a=atoms.find(var);
		if(a->second.getDescriptor()==Atom::NUMERICAL_VARIABLE){

			atoms[var]=val;
			return	true;
		}

		float32	multiplier=var.getMultiplier();
		float32	min=a->second.asFloat()*(1-multiplier);
		float32	max=a->second.asFloat()*(1+multiplier);
		if(val.asFloat()<min	||	val.asFloat()>max)	//	at least one value differs from an existing binding.
			return	false;
		return	true;
	}

	bool	BindingMap::bind_boolean_variable(Atom	val,Atom	var){

		UNORDERED_MAP<Atom,Atom>::const_iterator	a=atoms.find(var);
		if(a->second.getDescriptor()==Atom::BOOLEAN_VARIABLE){

			atoms[var]=val;
			return	true;
		}

		if(val.asBoolean()!=a->second.asBoolean())	//	at least one value differs from an existing binding.
			return	false;
		return	true;
	}

	bool	BindingMap::bind_structural_variable(Atom	*val,Atom	var){

		uint16	val_size=val->getAtomCount();
		UNORDERED_MAP<Atom,std::vector<Atom> >::iterator	s=structures.find(var);
		if(s->second[1].getDescriptor()==Atom::STRUCTURAL_VARIABLE){

			for(uint16	i=0;i<=val_size;++i)
				s->second[i]=val[i];
			return	true;
		}
		
		switch(s->second[0].getDescriptor()){
		case	Atom::TIMESTAMP:{	//	tolerance is used.

			float32	multiplier=var.getMultiplier();
			uint64	vt=Utils::GetTimestamp(&s->second[0]);
			uint64	now=Now();
			uint64	delta_t;
			if(now>vt)
				delta_t=now-vt;
			else
				delta_t=vt-now;
			uint64	min=delta_t*(1-multiplier);
			uint64	max=delta_t*(1+multiplier);
			uint64	t=Utils::GetTimestamp(val);
			if(t<now)
				t=now-t;
			else
				t=t-now;
			if(t<min	||	t>max)	//	the value differs from an existing binding.
				return	false;
			return	true;
		}case	Atom::STRING:	//	tolerance is not used.
			for(uint16	i=1;i<=val_size;++i){

				if(val[i]!=s->second[i])	//	part of the value differs from an existing binding.
					return	false;
			}
			return	true;
		case	Atom::OBJECT:	//	tolerance is used to compare numerical structure members.
			for(uint16	i=1;i<=val_size;++i){

				float32	multiplier=var.getMultiplier();
				if(val[i].isFloat()){

					float32	min=s->second[i].asFloat()*(1-multiplier);
					float32	max=s->second[i].asFloat()*(1+multiplier);
					float	v=val[i].asFloat();
					if(v<min	||	v>max)	//	at least one value differs from an existing binding.
						return	false;
				}else	if(val[i]!=s->second[i])	//	at least one value differs from an existing binding.
					return	false;
			}
			return	true;
		}
	}

	bool	BindingMap::bind_object_variable(Code	*val,Code	*var){

		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	o=objects.find(var);
		if(o->second->code(0).asOpcode()==Opcodes::Var){

			objects[var]=val;
			return	true;
		}

		if(o->second!=val)	//	at least one value differs from an existing binding.
			return	false;
		return	true;
	}

	void	BindingMap::copy(Code	*object,uint16	index)	const{

		uint16	args_size=objects.size()+atoms.size()+structures.size();
		object->code(index)=Atom::Set(args_size);

		uint16	code_index=index+1;
		uint16	ref_index=1;
		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	o;
		for(o=objects.begin();o!=objects.end();++o,++ref_index,++code_index){

			object->code(code_index)=Atom::RPointer(ref_index);
			object->set_reference(ref_index,o->second);
		}

		uint16	extent_index=index+args_size+1;
		UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
		for(s=structures.begin();s!=structures.end();++s,++code_index){

			object->code(code_index)=Atom::IPointer(extent_index);
			for(uint16	j=0;j<=s->second[0].getAtomCount();++j,++extent_index)
				object->code(extent_index)=s->second[j];
		}

		UNORDERED_MAP<Atom,Atom>::const_iterator	a;
		for(a=atoms.begin();a!=atoms.end();++a,++code_index)
			object->code(code_index)=a->second;
	}

	void	BindingMap::load(const	Code	*object){	//	source is icst or imdl.

		uint16	var_set_index=object->code(I_HLP_ARGS).asIndex();
		uint16	var_count=object->code(var_set_index).getAtomCount();

		UNORDERED_MAP<Code	*,P<Code> >::iterator			o=objects.begin();
		UNORDERED_MAP<Atom,std::vector<Atom> >::iterator	s=structures.begin();
		UNORDERED_MAP<Atom,Atom>::iterator					a=atoms.begin();
		
		for(uint16	i=1;i<var_count;++i){

			Atom	atom=object->code(var_set_index+i);
			switch(atom.getDescriptor()){
			case	Atom::R_PTR:
				o->second=object->get_reference(atom.asIndex());
				++o;
				break;
			case	Atom::I_PTR:{
				std::vector<Atom>	val;
				uint16	s_index=atom.asIndex();
				for(uint16	j=0;j<=object->code(s_index).getAtomCount();++j)
					val.push_back(object->code(s_index+j));
				s->second=val;
				break;
			}case	Atom::NUMERICAL_VARIABLE:
			case	Atom::BOOLEAN_VARIABLE:
				a->second=atom;
				++a;
				break;
			}
		}
	}
}