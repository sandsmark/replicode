//	loader_module.cpp
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

#include "loader_module.h"

#include	"image.h"
#include	"preprocessor.h"
#include	"compiler.h"


using	namespace	r_comp;

LOAD_MODULE(Loader)

void	Loader::initialize(){

	image=NULL;
}

void	Loader::finalize(){
}

void	Loader::compile(std::string		&filename){

	std::string		error;
	std::ifstream	source_code(filename.c_str());	//	ANSI encoding (not Unicode)

	r_comp::Image		*_image=new	r_comp::Image();	//	compiler input; definition segment filled by preprocessor

	if(!source_code.good()){

		std::cout<<"error: unable to load file "<<filename<<std::endl;
		return;
	}

	std::ostringstream		preprocessed_code_out;
	Preprocessor			preprocessor;
	if(!preprocessor.process(&_image->definition_segment,&source_code,&preprocessed_code_out,&error)){

		std::cout<<error;
		return;
	}
	source_code.close();

	std::istringstream	preprocessed_code_in(preprocessed_code_out.str());

	Compiler	compiler;
	if(!compiler.compile(&preprocessed_code_in,_image,&error,false)){

		std::streampos	i=preprocessed_code_in.tellg();
		std::cout.write(preprocessed_code_in.str().c_str(),i);
		std::cout<<" <- "<<error<<std::endl;
		delete	_image;
		return;
	}else{

		image=_image->serialize<ImageMessage>();	
		//image->trace();
		source_code.close();
	}
}