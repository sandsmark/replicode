//	object.cpp
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

#include	"object.h"
#include	"group.h"
#include	"opcodes.h"


namespace	r_exec{

	bool	IsNotification(Code	*object){

		switch(object->code(0).getDescriptor()){
		case	Atom::OBJECT:
			return	object->code(0).asOpcode()==Opcodes::MkActChg	||
					object->code(0).asOpcode()==Opcodes::MkAntiRdx	||
					object->code(0).asOpcode()==Opcodes::MkHighAct	||
					object->code(0).asOpcode()==Opcodes::MkHighSln	||
					object->code(0).asOpcode()==Opcodes::MkLowAct	||
					object->code(0).asOpcode()==Opcodes::MkLowRes	||
					object->code(0).asOpcode()==Opcodes::MkLowSln	||
					object->code(0).asOpcode()==Opcodes::MkNew		||
					object->code(0).asOpcode()==Opcodes::MkRdx		||
					object->code(0).asOpcode()==Opcodes::MkSlnChg;
		default:
			return	false;
		}
	}

	ObjectType	GetType(Code	*object){

		switch(object->code(0).getDescriptor()){
		case	Atom::INSTANTIATED_PROGRAM:
			if(object->get_reference(0)->code(0).asOpcode()==Opcodes::PGM){

				if(object->get_reference(0)->code(object->get_reference(0)->code(PGM_INPUTS).asIndex()).getAtomCount()>0)
					return	IPGM;
				else
					return	ANTI_IPGM;
			}else
				return	ANTI_IPGM;
		case	Atom::OBJECT:
			return	OBJECT;
		case	Atom::MARKER:
			return	MARKER;
		case	Atom::GROUP:
			return	GROUP;
		}
	}
}