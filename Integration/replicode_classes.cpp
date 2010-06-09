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


namespace	io{

	uint32	View::Opcode;

	const	char	*View::ClassName="view";

	r_code::View	*View::GetView(InputCode *input){

		r_code::View	*view=new	r_code::View();

		const	uint32	arity=5;	//	reminder: opcode not included in the arity
		uint32	write_index=0;
		uint32	extent_index=arity;

		view->code[write_index++]=r_code::Atom::Object(Opcode,arity);

		view->code[write_index++]=r_code::Atom::IPointer(extent_index);	//	ijt
		Timestamp	t1(Time::Get());
		t1.write(view->code,extent_index);

		view->code[write_index++]=r_code::Atom::Float(1.0);	//	sln

		view->code[write_index++]=r_code::Atom::IPointer(extent_index);	//	res
		Timestamp	t2(10000);
		t2.write(view->code,extent_index);

		view->code[write_index++]=r_code::Atom::RPointer(0);	//	stdin is the only reference
		//view->reference_set[0]=RetrieveStdin();

		view->code[write_index++]=r_code::Atom::Node(input->senderNodeID());	//	org

		return	view;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	Timestamp::Timestamp(uint64	t):t(t){
	}

	void	Timestamp::write(r_code::vector<r_code::Atom>	&code,uint32	&index)	const{

		code[index++]=Atom::Timestamp();
		code[index++]=t>>32;
		code[index++]=(t	&	0x00000000FFFFFFFF);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	uint32	Vec3::Opcode;

	const	char	*Vec3::ClassName="vec3";

	void	Vec3::write(r_code::vector<r_code::Atom>	&code,uint32	&index)	const{

		const	uint32	arity=3;

		code[index++]=r_code::Atom::Object(Opcode,arity);
		code[index++]=data[0];
		code[index++]=data[1];
		code[index++]=data[2];
	}
}