//	io_register.cpp
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

#include	"io_register.h"
#include	"r_mem_class.h"
#include	"replicode_classes.h"

#include	"preprocessor.h"


UNORDERED_MAP<uint32,IORegister>	IORegister::Register;

UNORDERED_MAP<std::string,r_code::Atom>	IORegister::Classes;

uint32	IORegister::LoadClasses(){

	//	load classes from the preprocessing std.replicode
	std::ifstream	source_code("C:/Work/Replicode/Test/user.classes.replicode");	//	TODO: make the path a parameter
	if(!source_code.good()){

		std::cout<<"error: unable to load user.classes.replicode"<<std::endl;
		return	1;
	}

	r_comp::Image			*_image=new	r_comp::Image();
	std::ostringstream		preprocessed_code_out;
	r_comp::Preprocessor	preprocessor;
	std::string				error;
	if(!preprocessor.process(&_image->definition_segment,&source_code,&preprocessed_code_out,&error)){

		std::cout<<error;
		delete	_image;
		return	1;
	}
	source_code.close();

	UNORDERED_MAP<std::string,r_comp::Class>::iterator it;
	for (it = _image->definition_segment.classes.begin(); it != _image->definition_segment.classes.end(); ++it) {
		Classes.insert(make_pair(it->first, it->second.atom));
		printf("CLASS %s=0x%08x\n", it->first.c_str(), it->second.atom.atom);
	}
	for (it = _image->definition_segment.sys_classes.begin(); it != _image->definition_segment.sys_classes.end(); ++it) {
		Classes.insert(make_pair(it->first, it->second.atom));
		printf("SYS %s=0x%08x\n", it->first.c_str(), it->second.atom.atom);
	}

	delete	_image;

	//	load registers and opcodes
	#define	REPLICODE_INPUT_CLASS(C)	LoadInputClass<C>();
	#define	REPLICODE_OUTPUT_CLASS(C)	LoadOutputClass<C>();
	#define	REPLICODE_UTIL_CLASS(C)		C::Opcode=GetOpcode(C::ClassName);
	#include	"replicode_class_def.h"

	return	0;
}
	
uint32	IORegister::ClassLoader=LoadClasses();

r_code::Object	*IORegister::GetObject(InputCode	*input){

	UNORDERED_MAP<uint32,IORegister>::iterator	it=Register.find(input->opcode());
	if(it==Register.end())
		return	NULL;
	return	it->second.objectBuilder(input);
}
	
OutputCode	*IORegister::GetCommand(r_code::Object	*object){

	UNORDERED_MAP<uint32,IORegister>::iterator	it=Register.find(object->opcode());
	if(it==Register.end())
		return	NULL;
	return	it->second.commandBuilder(object);
}
	
uint32	IORegister::GetOpcode(const	char	*class_name){

	UNORDERED_MAP<std::string,r_code::Atom>::iterator it=Classes.find(class_name);
	if(it==Classes.end())
		return	0;
	return	it->second.asOpcode();
}