//	usr_operators.h
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

#ifndef	usr_operators_h
#define	usr_operators_h

#include	"types.h"


extern	"C"{
void	dll_export	Init(OpcodeRetriever	r);	//	OpcodeRetriever allows the dll to retrieve opcodes from the r_rxec dll without referencing the corresponding STL structures.

//	Operators		//////////////////////////////////////////////////////////////////////////////

uint16	dll_export	GetOperatorCount();
void	dll_export	GetOperatorName(char	*op_name);

//	CPP Programs	//////////////////////////////////////////////////////////////////////////////

uint16	dll_export	GetProgramCount();
void	dll_export	GetProgramName(char	*pgm_name);

//	Callbacks		//////////////////////////////////////////////////////////////////////////////

uint16	dll_export	GetCallbackCount();
void	dll_export	GetCallbackName(char	*callback_name);
}

#include	"./Vec3/vec3.h"
#include	"./TestProgram/test_program.h"
#include	"./Correlator/correlator.h"
#include	"./Callbacks/callbacks.h"


#endif
