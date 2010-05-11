//	r_mem_module.cpp
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

#include "r_mem_module.h"

#include	"image.h"
#include	"decompiler.h"


LOAD_MODULE(RMem)

void	RMem::initialize(){

	std::vector<r_code::Object	*>	initial_objects;	//	empty

	//	build the rMem
	mem=r_exec::Mem::create(
		10000, // resilience update period: 10ms
		10000, // base update period: 10ms
		IORegister::Classes,
		initial_objects,
		NULL
	);
}

void	RMem::finalize(){

	delete	mem;
}

void	RMem::decompile(r_comp::Image	*image){

	r_comp::Decompiler	decompiler;
	std::ostringstream	decompiled_code;
	decompiler.decompile(image,&decompiled_code);
	std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
}

void	RMem::inject(r_comp::Image	*image){

	r_code::vector<r_code::Object	*>	objects;
	*image>>objects;
	for(uint32	i=0;i<objects.size();++i){

		r_code::Object	*object=objects[i];
		inject(object,NODE->id());
	}
}

void	RMem::inject(r_code::Object	*object,uint16	nodeID){

	//	an object may have several views: inject the object once per view
	//for(uint32	i=0;i<object->view_set.size();++i)
	//	mem->receive(object,object->view_set[i],nodeID,r_exec::ObjectReceiver::INPUT_GROUP);
}