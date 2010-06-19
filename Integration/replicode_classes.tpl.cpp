//	replicode_classes.tpl.cpp
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

template<class	V>	const	char	*MkVal<V>::ClassName="mk.val";
template<class	V>	uint16			MkVal<V>::Opcode;

template<class	V>	CodePayload	*MkVal<V>::New(CodePayload	*entity,CodePayload	*attribute,V	value){

	CodePayload	*r=new(14+2)	CodePayload(14);

	uint16	i=0;
	r->data(++i)=Atom::RPointer(0);				//	1	ptr to object.
	r->data(++i)=Atom::RPointer(1);				//	2	ptr to attribute.
	r->data(++i)=Atom::IPointer(8);				//	3	iptr to vec3.
	r->data(++i)=Atom::View();					//	4	vw.
	r->data(++i)=Atom::IPointer(0);				//	5	iptr to vws.
	r->data(++i)=Atom::IPointer(0);				//	6	iptr to mks.
	r->data(++i)=Atom::Float(1);				//	7	psln_thr.
	++i;
	value.write(r->data(),i);					//	writes 4 atoms: 8 -> 11
	r->data(++i)=Atom::Set(0);					//	12	vws.
	r->data(++i)=Atom::Set(0);					//	13	mks.

	r->data(++i).atom=(uint32)entity;
	r->data(++i).atom=(uint32)attribute;
	
	return	r;
}