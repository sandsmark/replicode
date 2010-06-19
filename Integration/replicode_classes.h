//	replicode_classes.h
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

#ifndef	replicode_classes_h
#define	replicode_classes_h

#include	"r_mem_class.h"


class	RMem;

//	Standard classes.

class	Entity{
public:
	static	const	char		*ClassName;
	static			uint16		Opcode;
	static			CodePayload	*New();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

class	Vec3{
public:
	static	const	char	*ClassName;
	static			uint16	Opcode;
	static	const	uint16	CodeSize;

	float32	data[3];	//	[x|y|z].

	Vec3(float32	x,float32	y,float32	z);

	void	write(Atom	*code,uint16	&index)	const;	//	writes the data at the specified index in the given array of atoms; updates the index to the last writing location.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

//	User-defined sys classes.
//	Prototype:
//	class	X{
//	public:
//		static	const	char	*ClassName;	//	initilaize with the name as define in user.classes.replicode.
//		static	uint16			Opcode;		//	will be initialized by the IORegister.
//		static					CodePayload	*New(... args ...);	//	executed by IO modules for sending.
//	};

template<class	V>	class	MkVal{
public:
	static	const	char		*ClassName;
	static			uint16		Opcode;
	static			CodePayload	*New(CodePayload	*entity,CodePayload	*attribute,V	value);
};


#include	"replicode_classes.tpl.cpp"


#endif