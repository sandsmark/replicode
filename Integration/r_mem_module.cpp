//	r_mem_module.cpp
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

#include "r_mem_module.h"

#include	"image.h"
#include	"decompiler.h"


LOAD_MODULE(RMem)

void	RMem::initialize(){

	r_exec::Mem<RObject>::init(100000,2,2);

	r_code::vector<r_code::Code	*>	objects;
	r_exec::Seed.getObjects(this,objects);
	r_exec::Mem<RObject>::load(objects.as_std());
}

void	RMem::finalize(){
}

void	RMem::decompile(r_comp::Image	*image){

	r_comp::Decompiler	decompiler;
	std::ostringstream	decompiled_code;
	decompiler.init(&r_exec::Metadata);
	decompiler.decompile(image,&decompiled_code);
	std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
}

void	RMem::load(r_comp::Image	*image){

	//r_exec::Mem<RObject>::stop();

	decompile(image);	//	for debugging.

	r_code::vector<r_code::Code	*>	objects;
	image->getObjects(this,objects);

	//r_exec::Mem<RObject>::load(objects.as_std());
	//r_exec::Mem<RObject>::start();
}

void	RMem::inject(RObject	*object,uint16	nodeID,STDGroupID	destination){

	//	TODO.
	//	inject references first: build stems for the CodePayloads.
	//	push all the created RObjects in a list, and inject in reverse.

/*	OLD CODE
	r_exec::View	*view=new	r_exec::View();

	const	uint32	arity=6;	//	reminder: opcode not included in the arity
	uint16	write_index=0;
	uint16	extent_index=arity;

	view->code(write_index++)=Atom::SSet(r_exec::View::ViewOpcode,arity);
	view->code(write_index++)=Atom::UndefinedFloat();			//	oid; will be set by the rMem.
	view->code(write_index++)=Atom::IPointer(extent_index);		//	ijt; will be set by the rMem.
	Timestamp::Set(&view->code(view->code(write_index-1).asIndex()),r_exec::Now());
	view->code(write_index++)=Atom::Float(1.0);					//	sln.
	view->code(write_index++)=Atom::Float(1.0);					//	res is set to 1 upr of the destination group.
	view->code(write_index++)=Atom::RPointer(0);				//	stdin is the only reference.
	view->code(write_index++)=Atom::Node(nodeID);				//	org.
	if(destination==r_exec::_Mem::STDIN)
		view->references[0]=r_exec::Mem<RObject>::get_stdin();
	else
		view->references[0]=r_exec::Mem<RObject>::get_stdin();

	r_exec::Mem<RObject>::inject(object,view);
*/}