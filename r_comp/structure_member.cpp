//	structure_member.cpp
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

#include	"structure_member.h"
#include	"compiler.h"


namespace	r_comp{

	StructureMember::StructureMember(){
	}

	StructureMember::StructureMember(_Read		r,
									std::string	m,
									std::string	p,
									Iteration	i):_read(r),
													name(m),
													_class(p),
													iteration(i){

				if(_read==&Compiler::read_any)			type=ANY;
		else	if(_read==&Compiler::read_number)		type=NUMBER;
		else	if(_read==&Compiler::read_timestamp)	type=TIMESTAMP;
		else	if(_read==&Compiler::read_boolean)		type=BOOLEAN;
		else	if(_read==&Compiler::read_string)		type=STRING;
		else	if(_read==&Compiler::read_node)			type=NODE_ID;
		else	if(_read==&Compiler::read_device)		type=DEVICE_ID;
		else	if(_read==&Compiler::read_function)		type=FUNCTION_ID;
		else	if(_read==&Compiler::read_expression)	type=ANY;
		else	if(_read==&Compiler::read_set)			type=ReturnType::SET;
	}
		
	Class	*StructureMember::get_class(DefinitionSegment	*segment)	const{
		
		return	_class==""?NULL:&segment->classes.find(_class)->second;
	}
	
	ReturnType	StructureMember::get_return_type()	const{
		
		return	type;
	}

	bool	StructureMember::used_as_expression()	const{
		
		return	iteration==I_EXPRESSION;
	}

	StructureMember::Iteration	StructureMember::getIteration()	const{

		return	iteration;
	}

	_Read	StructureMember::read()	const{

		return	_read;
	}

	void	StructureMember::write(word32	*storage)	const{

		if(_read==&Compiler::read_any)
			storage[0]=R_ANY;
		else	if(_read==&Compiler::read_number)
			storage[0]=R_NUMBER;
		else	if(_read==&Compiler::read_timestamp)
			storage[0]=R_TIMESTAMP;
		else	if(_read==&Compiler::read_boolean)
			storage[0]=R_BOOLEAN;
		else	if(_read==&Compiler::read_string)
			storage[0]=R_STRING;
		else	if(_read==&Compiler::read_node)
			storage[0]=R_NODE;
		else	if(_read==&Compiler::read_device)
			storage[0]=R_DEVICE;
		else	if(_read==&Compiler::read_function)
			storage[0]=R_FUNCTION;
		else	if(_read==&Compiler::read_expression)
			storage[0]=R_EXPRESSION;
		else	if(_read==&Compiler::read_set)
			storage[0]=R_SET;
		else	if(_read==&Compiler::read_view)
			storage[0]=R_VIEW;
		else	if(_read==&Compiler::read_mks)
			storage[0]=R_MKS;
		else	if(_read==&Compiler::read_vws)
			storage[0]=R_VWS;
		uint32	offset=1;
		storage[offset++]=type;
		r_code::Write(storage+offset,_class);
		offset+=r_code::GetSize(_class);
		storage[offset++]=iteration;
		r_code::Write(storage+offset,name);
	}

	void	StructureMember::read(word32	*storage){
		
		switch(storage[0]){
		case	R_ANY:			_read=&Compiler::read_any;	break;
		case	R_NUMBER:		_read=&Compiler::read_number;	break;
		case	R_TIMESTAMP:	_read=&Compiler::read_timestamp;	break;
		case	R_BOOLEAN:		_read=&Compiler::read_boolean;	break;
		case	R_STRING:		_read=&Compiler::read_string;	break;
		case	R_NODE:			_read=&Compiler::read_node;	break;
		case	R_DEVICE:		_read=&Compiler::read_device;	break;
		case	R_FUNCTION:		_read=&Compiler::read_function;	break;
		case	R_EXPRESSION:	_read=&Compiler::read_expression;	break;
		case	R_SET:			_read=&Compiler::read_set;	break;
		case	R_VIEW:			_read=&Compiler::read_view;	break;
		case	R_MKS:			_read=&Compiler::read_mks;	break;
		case	R_VWS:			_read=&Compiler::read_vws;	break;
		}
		uint32	offset=1;
		type=(ReturnType)storage[offset++];
		r_code::Read(storage+offset,_class);
		offset+=r_code::GetSize(_class);
		iteration=(Iteration)storage[offset++];
		r_code::Read(storage+offset,name);
	}

	uint32	StructureMember::getSize(){	//	see segments.cpp for the RAM layout

		uint32	size=3;	//	read ID, return type, iteration
		size+=r_code::GetSize(_class);
		size+=r_code::GetSize(name);
		return	size;
	}
}