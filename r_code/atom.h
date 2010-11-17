//	atom.h
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

#ifndef	r_code_atom_h
#define	r_code_atom_h

#include	"../../CoreLibrary/trunk/CoreLibrary/types.h"


using	namespace	core;

namespace	r_code{

	//	Opcodes on 12 bits.
	//	Indices on 12 bits.
	//	Element count on 8 bits.
	//	To define bigger constructs (e.g. large matrices), define hooks to RAM (and derive classes from Object).
	class	dll_export	Atom{
	private:	//	trace utilities.
		static	uint8	Members_to_go;
		static	uint8	Timestamp_data;
		static	uint8	String_data;
		static	uint8	Char_count;
		void	write_indents()	const;
	public:
		typedef	enum{
			NIL=0x80,
			BOOLEAN_=0x81,
			WILDCARD=0x82,
			T_WILDCARD=0x83,
			I_PTR=0x84,			// internal pointer.
			R_PTR=0x85,			// reference pointer.
			VL_PTR=0x86,		// value pointer.
			IPGM_PTR=0x87,		// r_exec internal: index of data of a tpl arg held by an ipgm.
			IN_OBJ_PTR=0x88,	// r_exec internal: index of data held by the object held by an input view.
			VALUE_PTR=0x89,		// r_exec internal: index of data held by the overlay's value array.
			PROD_PTR=0x8A,		// r_exec internal: index of data held by the overlay's production array.
			OUT_OBJ_PTR=0x8B,	// r_exec internal: index of data held by a newly produced object.
			THIS=0x90,			// this pointer.
			VIEW=0x91,
			MKS=0x92,
			VWS=0x93,
			NODE=0xA0,
			DEVICE=0xA1,
			DEVICE_FUNCTION=0xA2,
			C_PTR =0xC0,		// chain pointer.
			SET=0xC1,
			S_SET=0xC2,			// structured set.
			OBJECT=0xC3,
			MARKER=0xC4,
			OPERATOR=0xC5,
			STRING=0xC6,
			TIMESTAMP=0xC7,
			INSTANTIATED_PROGRAM=0xC8,
			GROUP=0xC9,
			REDUCTION_GROUP=0xCA,
			INSTANTIATED_CPP_PROGRAM=0xCB
		}Type;
		// encoders
		static	Atom	Float(float32 f);	//	IEEE 754 32 bits encoding; shifted by 1 to the right (loss of precison).
		static	Atom	PlusInfinity();
		static	Atom	MinusInfinity();
		static	Atom	UndefinedFloat();
		static	Atom	Nil();
		static	Atom	Boolean(bool value);
		static	Atom	UndefinedBoolean();
		static	Atom	Wildcard(uint16	opcode=0x00);
		static	Atom	TailWildcard();
		static	Atom	IPointer(uint16 index);
		static	Atom	RPointer(uint16 index);
		static	Atom	VLPointer(uint16 index,uint16	cast_opcode=0x0FFF);
		static	Atom	IPGMPointer(uint16 index);
		static	Atom	InObjPointer(uint8	inputIndex,uint16 index);
		static	Atom	OutObjPointer(uint16 index);
		static	Atom	ValuePointer(uint16 index);
		static	Atom	ProductionPointer(uint16 index);
		static	Atom	This();
		static	Atom	View();
		static	Atom	Mks();
		static	Atom	Vws();
		static	Atom	Node(uint8 nodeID);
		static	Atom	UndefinedNode();
		static	Atom	Device(uint8 nodeID,uint8 classID,uint8 devID);
		static	Atom	UndefinedDevice();
		static	Atom	DeviceFunction(uint16 opcode);
		static	Atom	UndefinedDeviceFunction();
		static	Atom	CPointer(uint8 elementCount);
		static	Atom	SSet(uint16 opcode,uint8 elementCount);
		static	Atom	Set(uint8 elementCount);
		static	Atom	Object(uint16 opcode,uint8 arity);
		static	Atom	Marker(uint16 opcode,uint8 arity);
		static	Atom	Operator(uint16 opcode,uint8 arity);
		static	Atom	String(uint8 characterCount);
		static	Atom	UndefinedString();
		static	Atom	Timestamp();
		static	Atom	UndefinedTimestamp();
		static	Atom	InstantiatedProgram(uint16 opcode,uint8 arity);
		static	Atom	Group(uint16 opcode,uint8 arity);
		static	Atom	ReductionGroup(uint16 opcode,uint8 arity);
		static	Atom	InstantiatedCPPProgram(uint16 opcode,uint8 arity);

		Atom(uint32	a=0xFFFFFFFF);
		~Atom();

		Atom	&operator	=(const	Atom&	a);
		bool	operator	==(const	Atom&	a)	const;
		bool	operator	!=(const	Atom&	a)	const;

		uint32	atom;

		// decoders
		uint8	getDescriptor()	const;
		bool	isPointer()		const;	// returns true for all pointer types.
										// incl. index, this and view.
		bool	isStructural()	const;
		bool	isFloat()		const;
		bool	readsAsNil()	const;	// returns true for all undefined values.
		float32	asFloat()		const;
		bool	asBoolean()		const;
		uint16	asIndex()		const;	// applicable to internal, view, reference,
										// and value pointers.
		uint8	asInputIndex()	const;	// applicable to IN_OBJ_PTR.
		uint16	asOpcode()		const;
		uint8	asCastOpcode()	const;	// applicable to VL_PTR.
		uint8	getAtomCount()	const;	// arity of operators and
										// objects/markers/structured sets,
										// number of atoms in pointers chains,
										// number of blocks of characters in
										// strings.
		uint8	getNodeID()		const;	// applicable to nodes and devices.
		uint8	getClassID()	const;	// applicable to devices.
		uint8	getDeviceID()	const;	// applicable to devices.

		void	trace()	const;
		static	void	Trace(Atom	*base,uint16	count);
	};
}


#include	"atom.inline.cpp"


#endif
