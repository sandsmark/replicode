//	usr_operators.cpp
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

#include	"usr_operators.h"

#include	<iostream>
#include	<cmath>


uint16	Vec3Opcode;

////////////////////////////////////////////////////////////////////////////////

bool	add(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.head().asOpcode()==Vec3Opcode	&&	rhs.head().asOpcode()==Vec3Opcode){

		index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float(lhs.getChild(1).head().asFloat()+rhs.getChild(1).head().asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getChild(2).head().asFloat()+rhs.getChild(2).head().asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getChild(3).head().asFloat()+rhs.getChild(3).head().asFloat()));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	sub(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.head().asOpcode()==Vec3Opcode	&&	rhs.head().asOpcode()==Vec3Opcode){

		index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float(lhs.getChild(1).head().asFloat()-rhs.getChild(1).head().asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getChild(2).head().asFloat()-rhs.getChild(2).head().asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getChild(3).head().asFloat()-rhs.getChild(3).head().asFloat()));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	mul(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.head().isFloat()){

		if(rhs.head().asOpcode()==Vec3Opcode){

			index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float(lhs.head().asFloat()*rhs.getChild(1).head().asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.head().asFloat()*rhs.getChild(2).head().asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.head().asFloat()*rhs.getChild(3).head().asFloat()));
			return	true;
		}
	}else	if(lhs.head().asOpcode()==Vec3Opcode){

		if(rhs.head().isFloat()){

			index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float(lhs.getChild(1).head().asFloat()-rhs.head().asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.getChild(2).head().asFloat()-rhs.head().asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.getChild(3).head().asFloat()-rhs.head().asFloat()));
			return	true;
		}
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	dis(const	r_exec::Context	&context,uint16	&index){	

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.head().asOpcode()==Vec3Opcode	&&	rhs.head().asOpcode()==Vec3Opcode){

		float32	d1=lhs.getChild(1).head().asFloat()-rhs.getChild(1).head().asFloat();
		float32	d2=lhs.getChild(2).head().asFloat()-rhs.getChild(2).head().asFloat();
		float32	d3=lhs.getChild(3).head().asFloat()-rhs.getChild(3).head().asFloat();

		float32	norm2=d1*d1+d2*d2+d3*d3;
		index=context.setAtomicResult(Atom::Float(sqrt(norm2)));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

void	Init(OpcodeRetriever	r){

	const	char	*vec3="vec3";

	Vec3Opcode=r(vec3);

	std::cout<<"usr operators initialized"<<std::endl;
}

uint16	GetOperatorCount(){

	return	4;
}

void	GetOperator(Operator	&op,char	*op_name){

	static	uint16	op_index=0;

	if(op_index==0){

		op=add;
		std::string	s="add";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==1){

		op=sub;
		std::string	s="sub";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==2){

		op=mul;
		std::string	s="mul";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==3){

		op=dis;
		std::string	s="dis";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}
}

