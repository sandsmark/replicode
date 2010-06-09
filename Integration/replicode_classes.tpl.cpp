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

namespace	io{
	
	template<class	V>	const	char	*MkVal<V>::ClassName="mk.val";

	template<class	V>	r_code::Object	*MkVal<V>::GetObject(InputCode	*input){

		MkVal<V>		*m=input->as<MkVal<V> >();
		r_code::Object	*object=new	r_code::Object();

		const	uint32	arity=7;	//	reminder: opcode not included in the arity
		uint32	write_index=0;
		uint32	extent_index=arity;

		object->code[write_index++]=r_code::Atom::Marker(m->opcode,arity);

		object->code[write_index++]=r_code::Atom::RPointer(0);	//	entity is the first ref
		//object->reference_set[0]=RetrieveObject(m->entityID);

		object->code[write_index++]=r_code::Atom::RPointer(1);	//	attribute is the second ref
		//object->reference_set[1]=RetrieveObject(m->attributeID);

		object->code[write_index++]=r_code::Atom::IPointer(extent_index);	//	vec3
		m->value.write(object->code,extent_index);

		object->code[write_index++]=r_code::Atom::View();	//	nil view
		
		object->code[write_index++]=r_code::Atom::IPointer(extent_index);	//	vws
		object->code[extent_index++]=r_code::Atom::Set(0);					//	empty set

		object->code[write_index++]=r_code::Atom::IPointer(extent_index);	//	mks
		object->code[extent_index++]=r_code::Atom::Set(0);					//	empty set

		object->code[write_index++]=r_code::Atom::Float(1.0);	//	psln_thr

		object->view_set[0]=View::GetView(input);

		return	object;
	}
}