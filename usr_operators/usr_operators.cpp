//	usr_operators.cpp
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

#include	"usr_operators.h"

#include	"../r_exec/init.h"

#include	<iostream>
#include	<cmath>


void	Init(OpcodeRetriever	r){

	Vec3::Init(r);

	std::cout<<"usr operators initialized"<<std::endl;
}

uint16	GetOperatorCount(){

	return	4;
}

void	GetOperatorName(char	*op_name){

	static	uint16	op_index=0;

	if(op_index==0){

		std::string	s="add";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==1){

		std::string	s="sub";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==2){

		std::string	s="mul";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==3){

		std::string	s="dis";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////

uint16	GetProgramCount(){

	return	3;
}

void	GetProgramName(char	*pgm_name){

	static	uint16	pgm_index=0;

	if(pgm_index==0){

		std::string	s="test_program";
		memcpy(pgm_name,s.c_str(),s.length());
		++pgm_index;
		return;
	}

	if(pgm_index==1){

		std::string	s="correlator";
		memcpy(pgm_name,s.c_str(),s.length());
		++pgm_index;
		return;
	}

	if(pgm_index==2){

		std::string	s="pattern_detector";
		memcpy(pgm_name,s.c_str(),s.length());
		++pgm_index;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////

uint16	GetCallbackCount(){

	return	1;
}

void	GetCallbackName(char	*callback_name){

	static	uint16	callback_index=0;

	if(callback_index==0){

		std::string	s="print";
		memcpy(callback_name,s.c_str(),s.length());
		++callback_index;
		return;
	}
}