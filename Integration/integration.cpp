//	integration.cpp
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

#include	"integration.h"
#include	"init.h"
#include	"replicode_classes.h"


using	namespace	r_code;

inline	uint64	_Now(){	return	module::Node::Get()->time();	}	//	have the rMem synchronized with all modules in the system.

void	Init(const	std::vector<word32>	&numbers,const	std::vector<std::string>	&strings){

	r_exec::Init(strings[0].c_str(),_Now,strings[1].c_str());
	#define	REPLICODE_CLASS(C)	C::Opcode=r_exec::GetOpcode(C::ClassName);
	#include	"replicode_class_def.h"

	//	add constant objects: all objects found except groups.
	r_code::vector<r_code::Code	*>	objects;
	r_exec::Seed.getObjects<RObject>(objects);
	for(uint16	i=0;i<objects.size();++i){

		if(objects[i]->code(0).asOpcode()!=r_exec::Opcodes::Group){

			module::Node::Get()->addConstantObject(((RObject	*)objects[i])->get_payload(),r_exec::GetAxiomName(i));
			delete	objects[i];	//	the stems are discarded; we olny keep the constant payloads.
		}
	}

	std::cout<<"integration library loaded"<<std::endl;
}
