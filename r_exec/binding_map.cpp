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
#include	"r_group.h"
#include	"binding_map.h"


namespace	r_exec{

	BindingMap::BindingMap():unbound_var_count(0){
	}

	void	BindingMap::clear(){

		objects.clear();
		atoms.clear();
		structures.clear();
	}

	void	BindingMap::init(BindingMap	*source){

		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	o;
		for(o=source->objects.begin();o!=source->objects.end();++o){

			objects[o->first]=o->second;
			if(o->second==NULL)
				++unbound_var_count;
		}
		UNORDERED_MAP<Atom,Atom>::const_iterator	a;
		for(a=source->atoms.begin();a!=source->atoms.end();++a){

			atoms[a->first]=a->second;
			if(!a->second)
				++unbound_var_count;
		}
		UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
		for(s=source->structures.begin();s!=source->structures.end();++s){
			
			structures[s->first]=s->second;
			if(s->second.size()==0)
				++unbound_var_count;
		}
	}

	void	BindingMap::add(BindingMap	*source){

		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	o;
		for(o=objects.begin();o!=objects.end();++o){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	_o=source->objects.find(o->first);
			if(_o!=source->objects.end()){

				objects[o->first]=_o->second;
				--unbound_var_count;
			}
		}
		UNORDERED_MAP<Atom,Atom>::const_iterator	a;
		for(a=atoms.begin();a!=atoms.end();++a){

			UNORDERED_MAP<Atom,Atom>::const_iterator	_a=source->atoms.find(a->first);
			if(_a!=source->atoms.end()){

				atoms[a->first]=_a->second;
				--unbound_var_count;
			}
		}
		UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
		for(s=structures.begin();s!=structures.end();++s){

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	_s=source->structures.find(s->first);
			if(_s!=source->structures.end()){

				structures[s->first]=_s->second;
				--unbound_var_count;
			}
		}
	}

	void	BindingMap::init(RGroup	*source){

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=source->variable_views.begin();v!=source->variable_views.end();++v)
			objects.insert(std::pair<Code	*,P<Code> >(v->second->object,NULL));
		for(uint16	i=0;i<source->numerical_variables.size();++i)
			atoms.insert(std::pair<Atom,Atom>(source->numerical_variables[i],Atom()));
		for(uint16	i=0;i<source->structural_variables.size();++i)
			structures.insert(std::pair<Atom,std::vector<Atom> >(source->structural_variables[i],std::vector<Atom>()));
		unbound_var_count=source->variable_views.size()+source->numerical_variables.size()+source->structural_variables.size();
	}

	void	BindingMap::bind_atom(const	Atom	&var,const	Atom	&val){

		atoms[var]=val;
		--unbound_var_count;
	}

	void	BindingMap::bind_structure(const	Atom	&var,const	std::vector<Atom>	&val){

		structures[var]=val;
		--unbound_var_count;
	}

	void	BindingMap::bind_object(Code	*var,Code	*val){

		objects[var]=val;
		--unbound_var_count;
	}

	void	BindingMap::unbind_atom(const	Atom	&var){

		atoms[var]=Atom();
		++unbound_var_count;
	}

	void	BindingMap::unbind_structure(const	Atom	&var){

		structures[var]=std::vector<Atom>();
		++unbound_var_count;
	}

	void	BindingMap::unbind_object(Code	*var){

		objects[var]=NULL;
		++unbound_var_count;
	}

	Code	*BindingMap::bind_object(Code	*original){

		if(original->code(0).asOpcode()==Opcodes::Var){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b=objects.find(original);
			if(b!=objects.end())
				return	b->second;
			else
				return	original;	//	no registered value; original is left unbound.
		}

		Code	*bound_object=_Mem::Get()->buildObject(original->code(0));
		for(uint16	i=0;i<original->code_size();){	//	patch code containing variables with actual values when they exist.
			
			Atom	a=original->code(i);
			switch(a.getDescriptor()){
			case	Atom::NUMERICAL_VARIABLE:{
				
				Atom	val=atoms[a];
				if(!val)	//	no registered value; original is left unbound.
					bound_object->code(i++)=a;
				else
					bound_object->code(i++)=val;
				break;
			}case	Atom::STRUCTURAL_VARIABLE:{

				std::vector<Atom>	val=structures[a];
				if(!val.size())	//	no registered value; original is left unbound.
					bound_object->code(i++)=a;
				else{
					
					if(bound_object->code(i-1).getDescriptor()==Atom::TIMESTAMP)	//	store the tolerance in the timestamp (used to create monitoring jobs for goals).
						bound_object->code(i-1).setTolerance(a.getTolerance());
					for(uint16	j=1;j<val.size();++j)
						bound_object->code(i++)=val[j];
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

	bool	BindingMap::needs_binding(Code	*original){

		for(uint16	i=0;i<original->code_size();++i){		//	find code containing variables.

			switch(original->code(i).getDescriptor()){
			case	Atom::NUMERICAL_VARIABLE:
			case	Atom::STRUCTURAL_VARIABLE:
				return	true;
			}
		}

		if(original->code(0).asOpcode()==Opcodes::Var){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b=objects.find(original);
			if(b!=objects.end())
				return	true;
			else
				return	false;
		}

		for(uint16	i=0;i<original->references_size();++i){

			if(needs_binding(original->get_reference(i)))
				return	true;
		}

		return	false;
	}
}