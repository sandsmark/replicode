//	r_mem_class.h
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

#ifndef	r_mem_class_h
#define	r_mem_class_h

#include	"mbrane.h"
#include	"image.h"
#include	"object.h"

#include	"io_register.h"


using	namespace	mBrane;
using	namespace	mBrane::sdk;
using	namespace	r_code;

template<class	U>	class	ImageCore:
public	Message<U,Memory>{
protected:
	uint32	_def_size;
	uint32	_map_size;
	uint32	_code_size;
	uint32	_reloc_size;
public:
	ImageCore():_def_size(0),_map_size(0),_code_size(0),_reloc_size(0){}
	uint32	def_size()		const{	return	_def_size;	}
	uint32	map_size()		const{	return	_map_size;	}
	uint32	code_size()		const{	return	_code_size;	}
	uint32	reloc_size()	const{	return	_reloc_size;}
};

template<class	U>	class	_Image:
public	CStorage<ImageCore<U>,word32>{
public:
	_Image():CStorage<ImageCore<U>,word32>(){}
	_Image(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size){
		_def_size=def_size;
		_map_size=map_size;
		_code_size=code_size;
		_reloc_size=reloc_size;
	}
};

class	ImageMessage:
public	Image<_Image<ImageMessage> >{
public:
	ImageMessage(){}
};

////////////////////////////////////////////////////////////////

template<class	U>	class	IOCode:
public	CStorage<Message<U,Memory>,word32>{
public:
	template<class	C>	static	U	*Build(){	//	convenience for tailoring the size to the embedded data size
		U	*u=new(sizeof(C)/sizeof(word32))	U();
		u->_data[0]=IORegister::GetOpcode(C::ClassName);	//	initilialize the opcode
		return	u;
	}
	template<class	O>	O	*as()	const{	return	(O	*)_data;	}	//	convenience for accessing the code
	uint32	opcode(){	return	_data[0];	}
};

class	InputCode:
public	IOCode<InputCode>{
};

class	OutputCode:
public	IOCode<OutputCode>{
};


#endif