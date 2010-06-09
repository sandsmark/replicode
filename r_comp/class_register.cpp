//	class_register.cpp
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

#include	"class_register.h"
#include	"preprocessor.h"


namespace	r_comp{

	UNORDERED_MAP<std::string,Atom>	ClassRegister::Opcodes;

	void	ClassRegister::LoadClasses(const	char	*path){

		//	load classes from the preprocessing of user.classes.replicode
		std::ifstream	source_code(path);
		if(!source_code.good()){

			std::cout<<"error: unable to load user.classes.replicode"<<std::endl;
			return;
		}

		r_comp::Image			*_image=new	r_comp::Image();
		std::ostringstream		preprocessed_code_out;
		r_comp::Preprocessor	preprocessor;
		std::string				error;
		if(!preprocessor.process(&_image->class_image,&source_code,&preprocessed_code_out,&error)){

			std::cout<<error;
			delete	_image;
			return;
		}
		source_code.close();

		UNORDERED_MAP<std::string,r_comp::Class>::iterator it;
		for (it=_image->class_image.classes.begin();it!=_image->class_image.classes.end();++it) {
			Opcodes.insert(make_pair(it->first,it->second.atom));
			printf("CLASS %s=0x%08x\n",it->first.c_str(),it->second.atom.atom);
		}
		for (it=_image->class_image.sys_classes.begin();it!=_image->class_image.sys_classes.end();++it) {
			Opcodes.insert(make_pair(it->first,it->second.atom));
			printf("SYS %s=0x%08x\n",it->first.c_str(),it->second.atom.atom);
		}

		delete	_image;
	}

	uint16	ClassRegister::GetOpcode(const	char	*class_name){

		std::string	s(class_name);
		UNORDERED_MAP<std::string,Atom>::const_iterator	it=Opcodes.find(s);
		if(it!=Opcodes.end())
			return	it->second.asOpcode();
		return	0xFFFF;
	}
}