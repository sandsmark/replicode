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
#include	"mem.h"


using	namespace	mBrane::sdk;
using	namespace	mBrane::sdk::payloads;

////	Image	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class	U>	class	ImageCore:
public	Message<U,Memory>{
protected:
	uint32	_map_size;
	uint32	_code_size;
	uint32	_reloc_size;
public:
	ImageCore():_map_size(0),_code_size(0),_reloc_size(0){}
	uint32	map_size()		const{	return	_map_size;	}
	uint32	code_size()		const{	return	_code_size;	}
	uint32	reloc_size()	const{	return	_reloc_size;}
};

template<class	U>	class	_Image:
public	CStorage<ImageCore<U>,word32>{
public:
	_Image():CStorage<ImageCore<U>,word32>(){}
	_Image(uint32	map_size,uint32	code_size,uint32	reloc_size){

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

////	RCode	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class	RObject;

template<class	U>	class	CodeHeader:
public	Message<U,Memory>{
public:
	CodeHeader():Message<U,Memory>(){}

	r_exec::_Mem::STDGroupID	destination;	//	unused except when received by RMems.
	uint32						code_size;		//	in the CStorage below.
	RObject						*stem;			//	used by rMems to retrieve the RObject/RMarker associated with a constant or cached shared object.
};

//	Stores the code and the references in one array.
//	That's the code that travels: from rMem to rMem or IO device to rMem.
//	When received by a rMem, a new RObject is created to hold the payload, except when the payload is a constant object (follow stem instead).
class	CodePayload:
public	CStorage<CodeHeader<CodePayload>,Atom>{
public:
	CodePayload(uint16	code_size,r_exec::_Mem::STDGroupID	destination=r_exec::_Mem::STDIN):CStorage<CodeHeader<CodePayload>,Atom>(){
	
		this->code_size=code_size;
		this->destination=destination;
		memset(_data,0,_capacity);
	}

	~CodePayload(){

		uint16	ref_count=_capacity-code_size;
		for(uint16	i=0;i<ref_count;++i)
			*((P<Code>	*)&data(code_size+i))=NULL;
	}

	uint16		ptrCount()	const{	return	_capacity-code_size;	}
	__Payload	*getPtr(uint16	i)	const;
	void		setPtr(uint16	i,__Payload	*p);
};

//	Instantiated on the RMem side (not on the device side).
//	This code does not travel.
//	same level of abstraction as r_code::Code.
class	RCode:
public	Code{
protected:
	P<CodePayload>	payload;
	Atom			*_code;		//	taken from the payload to avoid frequent indirections.
	uint16			_code_size;	//	idem.

	RCode():payload(NULL){}
public:
	Atom	&code(uint16	i){	return	_code[i];	}
	Atom	&code(uint16	i)	const{	return	_code[i];	}
	uint16	code_size()	const{	return	_code_size;	}
	Code	*get_reference(uint16	i)	const{	return	(Code	*)(&_code[_code_size+i]);	}
	void	set_reference(uint16	i,Code	*object){	(*(P<Code>	*)(&_code[_code_size+i]))=object;	}
	uint16	references_size()	const{	return	payload->getCapacity()-_code_size;	}

	virtual	~RCode(){}

	void	set_payload(CodePayload	*p,RObject	*s){

		payload=p;
		_code=payload->data();
		payload->stem=s;
		_code_size=payload->code_size;
	}
	CodePayload	*get_payload()	const{	return	payload;	}

	bool	is_compact()	const{	return	true;	}
};

//	Remote	object.
//	Same level of abstraction as r_exec::LObject.
//	This code does not travel: it acts like a stem holding the payload and lives inside the rMems; ejections actually eject the payload.
class	RObject:
public	r_exec::Object<RCode,RObject>{
public:
	static	bool	RequiresPacking(){	return	true;	}
	static	RObject	*Pack(Code	*object,r_code::Mem	*m){	//	no need to copy any views or markers, there are none (the object is freshly built).

		if(object->is_compact())	//	already an RObject.
			return	(RObject	*)object;

		RObject	*r=new	RObject(m);
		r->set_payload(new(object->code_size()+object->references_size())	CodePayload(object->code_size()),r);
		for(uint16	i=0;i<object->code_size();++i)
			r->code(i)=object->code(i);
		for(uint16	i=0;i<object->references_size();++i)
			r->set_reference(i,object->get_reference(i));
		return	r;
	}
	RObject(r_code::Mem	*m):r_exec::Object<RCode,RObject>(m){}
	RObject(r_code::SysObject	*source,r_code::Mem	*m=NULL):r_exec::Object<RCode,RObject>(m){
		
		set_payload(new(source->code.size()+source->references.size())	CodePayload(source->code.size()),this);
		load(source);
		build_views<r_exec::View>(source);
	}
	RObject(CodePayload	*p,r_code::Mem	*m):r_exec::Object<RCode,RObject>(m){

		set_payload(p,this);
	}
	~RObject(){}
};

//	Command sent by RMems to devices.
//	Defined as a stream: sid is set to the device id.
template<class	U>	class	CommandHeader:
public	StreamData<U,Memory>{
public:
	CommandHeader():StreamData<U,Memory>(){}

	uint32	code_size;	//	in the CStorage below.
};

//	Stores the code and the references in one array.
//	This code travels from rMems to IO devices.
class	Command:
public	CStorage<CommandHeader<Command>,Atom>{
public:
	Command(uint16	code_size):CStorage<CommandHeader<Command>,Atom>(){
	
		this->code_size=code_size;
		memset(_data,0,_capacity);
	}

	~Command(){

		uint16	ref_count=_capacity-code_size;
		for(uint16	i=0;i<ref_count;++i)
			*((P<Code>	*)&data(code_size+i))=NULL;
	}

	void	load(r_exec::LObject	*object){

		for(uint16	i=0;i<object->code_size();++i)
			data(i)=object->code(i);
		for(uint16	i=0;i<object->references_size();++i)
			*((P<Code>	*)&data(code_size+i))=object->get_reference(i);
	}
};


#endif