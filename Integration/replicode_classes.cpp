//	replicode_classes.cpp
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

#include	"replicode_classes.h"
#include	"../r_code/utils.h"


const	char	*Entity::ClassName="ent";
uint16	Entity::Opcode;

CodePayload	*Entity::New(){

	CodePayload	*r=new(2)	CodePayload(2);

	r->data(1)=Atom::Float(1);		//	psln_thr.
	
	return	r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

uint16	Vec3::Opcode;

const	char	*Vec3::ClassName="vec3";
const	uint16	Vec3::CodeSize=4;

Vec3::Vec3(float32	x,float32	y,float32	z){

	data[0]=x;
	data[1]=y;
	data[2]=z;
}

void	Vec3::write(Atom	*code,uint16	&index)	const{

	const	uint16	arity=3;
	code[index]=Atom::Object(Opcode,arity);
	code[++index]=Atom::Float(data[0]);
	code[++index]=Atom::Float(data[1]);
	code[++index]=Atom::Float(data[2]);
}