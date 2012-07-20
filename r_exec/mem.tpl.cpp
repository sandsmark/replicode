//	mem.tpl.cpp
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

#include	"init.h"
#include	"binding_map.h"
#include	"../r_code/replicode_defs.h"
#include	"operator.h"
#include	"factory.h"
#include	"cpp_programs.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	template<class	O>	Mem<O>::Mem():_Mem(){
	}

	template<class	O>	Mem<O>::~Mem(){
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	r_code::Code	*Mem<O>::build_object(r_code::SysObject	*source)	const{

		Atom	head=source->code[0];
		switch(head.getDescriptor()){
		case	Atom::GROUP:
			return	new	Group(source);
		default:{
			uint16	opcode=head.asOpcode();
			if(opcode==Opcodes::Fact)
				return	new	Fact(source);
			else	if(opcode==Opcodes::AntiFact)
				return	new	AntiFact(source);
			else	if(opcode==Opcodes::Goal)
				return	new	Goal(source);
			else	if(opcode==Opcodes::Pred)
				return	new	Pred(source);
			else	if(opcode==Opcodes::ICst)
				return	new	ICST(source);
			else	if(opcode==Opcodes::MkRdx)
				return	new	MkRdx(source);
			else	if(	opcode==Opcodes::MkActChg	||
						opcode==Opcodes::MkHighAct	||
						opcode==Opcodes::MkHighSln	||
						opcode==Opcodes::MkLowAct	||
						opcode==Opcodes::MkLowRes	||
						opcode==Opcodes::MkLowSln	||
						opcode==Opcodes::MkNew		||
						opcode==Opcodes::MkSlnChg	||
						opcode==Opcodes::Success	||
						opcode==Opcodes::Perf)
				return	new	r_code::LObject(source);
			else
				return	new	O(source);
		}
		}
	}

	template<class	O>	r_code::Code	*Mem<O>::_build_object(Atom	head)	const{

		r_code::Code	*object=new	O();
		object->code(0)=head;
		return	object;
	}

	template<class	O>	r_code::Code	*Mem<O>::build_object(Atom	head)	const{

		r_code::Code	*object;
		switch(head.getDescriptor()){
		case	Atom::GROUP:
			object=new	Group();
			break;
		default:{
			uint16	opcode=head.asOpcode();
			if(opcode==Opcodes::Fact)
				object=new	Fact();
			else	if(opcode==Opcodes::AntiFact)
				object=new	AntiFact();
			else	if(opcode==Opcodes::Pred)
				object=new	Pred();
			else	if(opcode==Opcodes::Goal)
				object=new	Goal();
			else	if(opcode==Opcodes::ICst)
				object=new	ICST();
			else	if(opcode==Opcodes::MkRdx)
				object=new	MkRdx();
			else	if(	opcode==Opcodes::MkActChg	||
						opcode==Opcodes::MkHighAct	||
						opcode==Opcodes::MkHighSln	||
						opcode==Opcodes::MkLowAct	||
						opcode==Opcodes::MkLowRes	||
						opcode==Opcodes::MkLowSln	||
						opcode==Opcodes::MkNew		||
						opcode==Opcodes::MkSlnChg	||
						opcode==Opcodes::Success	||
						opcode==Opcodes::Perf)
				object=new	r_code::LObject();
			else	if(O::RequiresPacking())
				object=new	r_code::LObject();	// temporary sand box for assembling code; will be packed into an O at injection time.
			else
				object=new	O();
			break;
		}
		}

		object->code(0)=head;
		return	object;
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	bool	Mem<O>::load(std::vector<r_code::Code	*>	*objects,
											uint32							stdin_oid,
											uint32							stdout_oid,
											uint32							self_oid){	// NB: no cov at init time.

		uint32	i;
		reduction_cores=new	ReductionCore	*[reduction_core_count];
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]=new	ReductionCore();
		time_cores=new	TimeCore	*[time_core_count];
		for(i=0;i<time_core_count;++i)
			time_cores[i]=new	TimeCore();

		last_oid=objects->size();

		// load root (always comes first).
		_root=(Group	*)(*objects)[0];
		this->objects.push_back((Code	*)_root);
		initial_groups.push_back(_root);

		for(uint32	i=1;i<objects->size();++i){	// skip root as it has no initial views.

			Code	*object=(*objects)[i];
			if(object->get_oid()==stdin_oid)
				_stdin=(Group	*)(*objects)[i];
			else	if(object->get_oid()==stdout_oid)
				_stdout=(Group	*)(*objects)[i];
			else	if(object->get_oid()==self_oid)
				_self=(O	*)(*objects)[i];

			switch(object->code(0).getDescriptor()){
			case	Atom::MODEL:
				unpack_hlp(object);
				//object->add_reference(NULL);	// classifier.
				break;
			case	Atom::COMPOSITE_STATE:
				unpack_hlp(object);
				break;
			case	Atom::INSTANTIATED_PROGRAM:	// refine the opcode depending on the inputs and the program type.
				if(object->get_reference(0)->code(0).asOpcode()==Opcodes::Pgm){

					if(object->get_reference(0)->code(object->get_reference(0)->code(PGM_INPUTS).asIndex()).getAtomCount()==0)
						object->code(0)=Atom::InstantiatedInputLessProgram(object->code(0).asOpcode(),object->code(0).getAtomCount());
				}else
					object->code(0)=Atom::InstantiatedAntiProgram(object->code(0).asOpcode(),object->code(0).getAtomCount());
				break;
			}

			UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	v;
			for(v=object->views.begin();v!=object->views.end();++v){

				//	init hosts' member_set.
				View	*view=(r_exec::View	*)*v;
				view->set_object(object);
				Group	*host=view->get_host();

				if(!host->load(view,object))
					return	false;
			}

			this->objects.push_back(object);
			object->is_registered=true;
			if(object->code(0).getDescriptor()==Atom::GROUP)
				initial_groups.push_back((Group	*)object);	// convenience to create initial update jobs - see start().
		}
		registered_object_count=this->objects.size();

		return	true;
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	Code	*Mem<O>::check_existence(Code	*object){

		if(object->code(0).getDescriptor()==Atom::GROUP)	// groups are always new.
			return	object;

		O	*_object;
		if(O::RequiresPacking())			// false if LObject, true for network-aware objects.
			_object=O::Pack(object,this);	// non compact form will be deleted (P<> in view) if not an instance of O; compact forms are left unchanged.
		else
			_object=(O	*)object;

		return	_object;
	}

	template<class	O>	void	Mem<O>::inject(O	*object,View	*view){

		view->set_object(object);
		inject_new_object(view);
	}
}