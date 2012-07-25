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

	template<class	O,class	S>	Mem<O,S>::Mem():S(){
	}

	template<class	O,class	S>	Mem<O,S>::~Mem(){
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	S>	r_code::Code	*Mem<O,S>::build_object(r_code::SysObject	*source)	const{

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

	template<class	O,class	S>	r_code::Code	*Mem<O,S>::_build_object(Atom	head)	const{

		r_code::Code	*object=new	O();
		object->code(0)=head;
		return	object;
	}

	template<class	O,class	S>	r_code::Code	*Mem<O,S>::build_object(Atom	head)	const{

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

	template<class	O,class	S>	Code	*Mem<O,S>::check_existence(Code	*object){

		if(object->code(0).getDescriptor()==Atom::GROUP)	// groups are always new.
			return	object;

		O	*_object;
		if(O::RequiresPacking())			// false if LObject, true for network-aware objects.
			_object=O::Pack(object,this);	// non compact form will be deleted (P<> in view) if not an instance of O; compact forms are left unchanged.
		else
			_object=(O	*)object;

		return	_object;
	}

	template<class	O,class	S>	void	Mem<O,S>::inject(O	*object,View	*view){

		view->set_object(object);
		inject_new_object(view);
	}
}