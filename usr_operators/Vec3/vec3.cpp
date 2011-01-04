//	vec3.cpp
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

#include	"vec3.h"

#include	"../r_exec/init.h"
#include	"../r_exec/mem.h"


uint16	Vec3Opcode;

////////////////////////////////////////////////////////////////////////////////

bool	add(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].asOpcode()==Vec3Opcode	&&	rhs[0].asOpcode()==Vec3Opcode){

		index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(1))[0].asFloat()+rhs.getChild(1)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(2))[0].asFloat()+rhs.getChild(2)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(3))[0].asFloat()+rhs.getChild(3)[0].asFloat()));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	sub(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].asOpcode()==Vec3Opcode	&&	rhs[0].asOpcode()==Vec3Opcode){

		index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(1))[0].asFloat()-rhs.getChild(1)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(2))[0].asFloat()-rhs.getChild(2)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(3))[0].asFloat()-rhs.getChild(3)[0].asFloat()));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	mul(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].isFloat()){

		if(rhs[0].asOpcode()==Vec3Opcode){

			index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float(lhs[0].asFloat()*(*rhs.getChild(1))[0].asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs[0].asFloat()*(*rhs.getChild(2))[0].asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs[0].asFloat()*(*rhs.getChild(3))[0].asFloat()));
			return	true;
		}
	}else	if(lhs[0].asOpcode()==Vec3Opcode){

		if(rhs[0].isFloat()){

			index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float((*lhs.getChild(1))[0].asFloat()-rhs[0].asFloat()));
			context.addCompoundResultPart(Atom::Float((*lhs.getChild(2))[0].asFloat()-rhs[0].asFloat()));
			context.addCompoundResultPart(Atom::Float((*lhs.getChild(3))[0].asFloat()-rhs[0].asFloat()));
			return	true;
		}
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	dis(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].asOpcode()==Vec3Opcode	&&	rhs[0].asOpcode()==Vec3Opcode){

		float32	d1=(*lhs.getChild(1))[0].asFloat()-(*rhs.getChild(1))[0].asFloat();
		float32	d2=(*lhs.getChild(2))[0].asFloat()-(*rhs.getChild(2))[0].asFloat();
		float32	d3=(*lhs.getChild(3))[0].asFloat()-(*rhs.getChild(3))[0].asFloat();

		float32	norm2=d1*d1+d2*d2+d3*d3;
		index=context.setAtomicResult(Atom::Float(sqrt(norm2)));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

void	Vec3::Init(OpcodeRetriever	r){

	const	char	*vec3="vec3";

	Vec3Opcode=r(vec3);
}