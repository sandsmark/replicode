//	segments.cpp
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

#include	"segments.h"

#include	<iostream>


namespace	r_comp{

	Reference::Reference(){
	}

	Reference::Reference(const	uint16	i,const	Class	&c):index(i),_class(c){
	}

	////////////////////////////////////////////////////////////////

	ClassImage::ClassImage(){
	}

	Class	*ClassImage::getClass(std::string	&class_name){

		UNORDERED_MAP<std::string,Class>::iterator	it=classes.find(class_name);
		if(it!=classes.end())
			return	&it->second;
		return	NULL;
	}

	Class	*ClassImage::getClass(uint16	opcode){

		return	&classes_by_opcodes[opcode];
	}
	
	void	ClassImage::write(word32	*data){
		
		data[0]=classes_by_opcodes.size();
		uint32	i;
		uint32	offset=1;
		for(i=0;i<classes_by_opcodes.size();++i){

			classes_by_opcodes[i].write(data+offset);
			offset+=classes_by_opcodes[i].getSize();
		}

		data[offset++]=classes.size();
		UNORDERED_MAP<std::string,Class>::iterator	it=classes.begin();
		for(;it!=classes.end();++it){

			r_code::Write(data+offset,it->first);
			offset+=r_code::GetSize(it->first);
			data[offset]=it->second.atom.asOpcode();
			offset++;
		}

		data[offset++]=sys_classes.size();
		it=sys_classes.begin();
		for(;it!=sys_classes.end();++it){

			r_code::Write(data+offset,it->first);
			offset+=r_code::GetSize(it->first);
			data[offset]=it->second.atom.asOpcode();
			offset++;
		}

		data[offset++]=class_names.size();
		for(i=0;i<class_names.size();++i){

			r_code::Write(data+offset,class_names[i]);
			offset+=r_code::GetSize(class_names[i]);
		}

		data[offset++]=operator_names.size();
		for(i=0;i<operator_names.size();++i){

			r_code::Write(data+offset,operator_names[i]);
			offset+=r_code::GetSize(operator_names[i]);
		}

		data[offset++]=function_names.size();
		for(i=0;i<function_names.size();++i){

			r_code::Write(data+offset,function_names[i]);
			offset+=r_code::GetSize(function_names[i]);
		}
	}
		
	void	ClassImage::read(word32	*data,uint32	size){
		
		uint32	class_count=data[0];
		uint32	i;
		uint32	offset=1;
		for(i=0;i<class_count;++i){

			Class	c;
			c.read(data+offset);
			classes_by_opcodes.push_back(c);
			offset+=c.getSize();
		}

		uint32	classes_count=data[offset++];
		for(i=0;i<classes_count;++i){

			std::string	s;
			r_code::Read(data+offset,s);
			offset+=r_code::GetSize(s);
			classes[s]=classes_by_opcodes[data[offset++]];
		}

		uint32	sys_classes_count=data[offset++];
		for(i=0;i<sys_classes_count;++i){

			std::string	s;
			r_code::Read(data+offset,s);
			offset+=r_code::GetSize(s);
			sys_classes[s]=classes_by_opcodes[data[offset++]];
		}

		uint32	class_names_count=data[offset++];
		for(i=0;i<class_names_count;++i){

			std::string	s;
			r_code::Read(data+offset,s);
			class_names.push_back(s);
			offset+=r_code::GetSize(s);
		}

		uint32	operator_names_count=data[offset++];
		for(i=0;i<operator_names_count;++i){

			std::string	s;
			r_code::Read(data+offset,s);
			operator_names.push_back(s);
			offset+=r_code::GetSize(s);
		}

		uint32	function_names_count=data[offset++];
		for(i=0;i<function_names_count;++i){

			std::string	s;
			r_code::Read(data+offset,s);
			function_names.push_back(s);
			offset+=r_code::GetSize(s);
		}
	}

	uint32	ClassImage::getSize(){

		return	getClassArraySize()+
				getClassesSize()+
				getSysClassesSize()+
				getClassNamesSize()+
				getOperatorNamesSize()+
				getFunctionNamesSize();
	}

	//	RAM layout:
	//	- class array
	//		- number of elements
	//		- list of classes:
	//			- atom
	//			- string (str_opcode)
	//			- return type
	//			- usage
	//			- things to read:
	//				- number of elements
	//				- list of structure members:
	//					- ID of a Compiler::*_Read function
	//					- return type
	//					- string (_class)
	//					- iteration
	//	- classes:
	//		- number of elements
	//		- list of pairs:
	//			- string
	//			- index of a class in the class array
	//	- sys_classes:
	//		- number of elements
	//		- list of pairs:
	//			- string
	//			- index of a class in the class array
	//	- class names:
	//		- number of elements
	//		- list of strings
	//	- operator names:
	//		- number of elements
	//		- list of strings
	//	- function names:
	//		- number of elements
	//		- list of strings
	//
	//	String layout:
	//		- size in word32
	//		- list of words: contain the charaacters; the last one is \0; some of the least significant bytes of the last word my be empty

	uint32	ClassImage::getClassArraySize(){

		uint32	size=1;	//	size of the array
		for(uint32	i=0;i<classes_by_opcodes.size();++i)
			size+=classes_by_opcodes[i].getSize();
		return	size;
	}

	uint32	ClassImage::getClassesSize(){

		uint32	size=1;	//	size of the hash table
		UNORDERED_MAP<std::string,Class>::iterator	it=classes.begin();
		for(;it!=classes.end();++it)
			size+=r_code::GetSize(it->first)+1;	//	+1: index to the class in the class array
		return	size;
	}

	uint32	ClassImage::getSysClassesSize(){

		uint32	size=1;	//	size of the hash table
		UNORDERED_MAP<std::string,Class>::iterator	it=sys_classes.begin();
		for(;it!=sys_classes.end();++it)
			size+=r_code::GetSize(it->first)+1;	//	+1: index to the class in the class array
		return	size;
	}

	uint32	ClassImage::getClassNamesSize(){

		uint32	size=1;	//	size of the vector
		for(uint32	i=0;i<class_names.size();++i)
			size+=r_code::GetSize(class_names[i]);
		return	size;
	}

	uint32	ClassImage::getOperatorNamesSize(){

		uint32	size=1;	//	size of the vector
		for(uint32	i=0;i<operator_names.size();++i)
			size+=r_code::GetSize(operator_names[i]);
		return	size;
	}

	uint32	ClassImage::getFunctionNamesSize(){

		uint32	size=1;	//	size of the vector
		for(uint32	i=0;i<function_names.size();++i)
			size+=r_code::GetSize(function_names[i]);
		return	size;
	}

	////////////////////////////////////////////////////////////////

	void	ObjectMap::shift(uint32	offset){

		for(uint32	i=0;i<objects.size();++i)
			objects[i]+=offset;
	}

	void	ObjectMap::write(word32	*data){

		for(uint32	i=0;i<objects.size();++i)
			data[i]=objects[i];
	}

	void	ObjectMap::read(word32	*data,uint32	size){

		for(uint32	i=0;i<size;++i)
			objects.push_back(data[i]);
	}

	uint32	ObjectMap::getSize()	const{

		return	objects.size();
	}

	////////////////////////////////////////////////////////////////

	CodeSegment::~CodeSegment(){

		for(uint32	i=0;i<objects.size();++i)
			delete	objects[i];
	}

	void	CodeSegment::write(word32	*data){

		uint32	offset=0;
		for(uint32	i=0;i<objects.size();++i){

			objects[i]->write(data+offset);
			offset+=objects[i]->getSize();
		}
	}

	void	CodeSegment::read(word32	*data,uint32	object_count){

		uint32	offset=0;
		for(uint32	i=0;i<object_count;++i){

			SysObject	*o=new	SysObject();
			o->read(data+offset);
			objects.push_back(o);
			offset+=o->getSize();
		}
	}

	uint32	CodeSegment::getSize(){

		uint32	size=0;
		for(uint32	i=0;i<objects.size();++i)
			size+=objects[i]->getSize();
		return	size;
	}

	////////////////////////////////////////////////////////////////

	RelocationSegment::PointerIndex::PointerIndex():object_index(0),pointer_index(0){
	}

	RelocationSegment::PointerIndex::PointerIndex(uint32	object_index,int32	view_index,uint32	pointer_index):object_index(object_index),view_index(view_index),pointer_index(pointer_index){
	}

	void	RelocationSegment::Entry::write(word32	*data){

		data[0]=pointer_indexes.size();
		for(uint32	i=0;i<pointer_indexes.size();++i){

			data[1+i*3]=pointer_indexes[i].object_index;
			data[2+i*3]=pointer_indexes[i].view_index;
			data[3+i*3]=pointer_indexes[i].pointer_index;
		}
	}

	void	RelocationSegment::Entry::read(word32	*data){

		for(uint32	i=0;i<(uint32)data[0];++i)
			pointer_indexes.push_back(PointerIndex(data[1+i*3],data[2+i*3],data[3+i*3]));
	}

	uint32	RelocationSegment::Entry::getSize()	const{

		return	1+pointer_indexes.size()*3;
	}

	void	RelocationSegment::addObjectReference(uint32	referenced_object_index,uint32	referencing_object_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,-1,reference_pointer_index));
	}

	void	RelocationSegment::addViewReference(uint32	referenced_object_index,uint32	referencing_object_index,int32	referencing_view_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,referencing_view_index,reference_pointer_index));
	}

	void	RelocationSegment::addMarkerReference(uint32	referenced_object_index,uint32	referencing_object_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,-2,reference_pointer_index));
	}
	
	void	RelocationSegment::write(word32	*data){

		data[0]=entries.size();
		uint32	offset=1;
		for(uint32	i=0;i<entries.size();++i){

			entries[i].write(data+offset);
			offset+=entries[i].getSize();
		}
	}

	void	RelocationSegment::read(word32	*data){

		uint32	entry_count=data[0];
		uint32	offset=1;
		for(uint32	i=0;i<entry_count;++i){

			Entry	e;
			e.read(data+offset);
			entries.push_back(e);
			offset+=e.getSize();
		}
	}

	uint32	RelocationSegment::getSize(){

		uint32	size=1;
		for(uint32	i=0;i<entries.size();++i)
			size+=entries[i].getSize();
		return	size;
	}

	////////////////////////////////////////////////////////////////

	class	NoMem:
	public	Mem{
	public:
		Object	*buildObject(SysObject	*source){
			return	new	Object(source);
		}
		Object	*buildGroup(SysObject	*source){
			return	new	Object(source);
		}
		Object	*buildInstantiatedProgram(SysObject	*source){
			return	new	Object(source);
		}
		Object	*buildMarker(SysObject	*source){
			return	new	Object(source);
		}
	};

	////////////////////////////////////////////////////////////////

	CodeImage::CodeImage():map_offset(0),mem(NULL){
	}

	CodeImage::~CodeImage(){
	}

	void	CodeImage::bind(Mem	*m){

		mem=m;
	}

	void	CodeImage::addObject(SysObject	*object){

		code_segment.objects.push_back(object);
		object_map.objects.push_back(map_offset);
		map_offset+=object->getSize();
	}

	CodeImage	&CodeImage::operator	<<(r_code::vector<Object	*>	&ram_objects){

		uint32	i;
		for(i=0;i<ram_objects.size();++i)
			*this<<ram_objects[i];
		return	*this;
	}

	CodeImage	&CodeImage::operator	<<(Object	*object){

		static	uint32	Last_index=0;

		UNORDERED_MAP<Object	*,uint32>::iterator	it=ptrs_to_indices.find(object);
		if(it!=ptrs_to_indices.end())	//	object already there
			return	*this;

		uint32	object_index;
		ptrs_to_indices[object]=object_index=Last_index++;
		SysObject	*sys_object=new	SysObject(object);
		addObject(sys_object);

		uint32	i;
		for(i=0;i<object->reference_set.size();++i)	//	follow reference pointers and recurse
			*this<<object->reference_set[i];
		for(i=0;i<object->marker_set.size();++i)	//	follow marker pointers and recurse
			*this<<object->marker_set[i];

		buildReferences(sys_object,object,object_index);
		return	*this;
	}

	void	CodeImage::buildReferences(SysObject	*sys_object,Object	*object,uint32	object_index){

		//	Translate pointers into indices: valuate the sys_object's references to object, incl. sys_object's view references and markers
		uint32	i;
		uint32	referenced_object_index;
		for(i=0;i<object->reference_set.size();++i){

			referenced_object_index=ptrs_to_indices.find(object->reference_set[i])->second;
			sys_object->reference_set.push_back(referenced_object_index);
			relocation_segment.addObjectReference(referenced_object_index,object_index,i);
		}
		for(i=0;i<object->view_set.size();++i)
			for(uint32	j=0;j<2;++j){	//	2 refs maximum; may be NULL.

				if(object->view_set[i]->reference_set[j]){

					referenced_object_index=ptrs_to_indices.find(object->view_set[i]->reference_set[j])->second;
					sys_object->view_set[i]->reference_set.push_back(referenced_object_index);
					relocation_segment.addViewReference(referenced_object_index,object_index,i,j);
				}
			}
		for(i=0;i<object->marker_set.size();++i){

			referenced_object_index=ptrs_to_indices.find(object->marker_set[i])->second;
			sys_object->marker_set.push_back(referenced_object_index);
			relocation_segment.addMarkerReference(referenced_object_index,object_index,i);
		}
	}

	void	CodeImage::getObjects(ClassImage	*class_image,r_code::vector<Object	*>	&ram_objects){

		bool	noMem=false;
		if(!mem){

			noMem=true;
			mem=new	NoMem();
		}

		uint32	i;
		for(i=0;i<code_segment.objects.size();++i){

			uint16	opcode=code_segment.objects[i]->code[0].asOpcode();
			if(opcode==class_image->classes.find("grp")->second.atom.asOpcode())
				ram_objects[i]=mem->buildGroup(code_segment.objects[i]);
			else	if(opcode==class_image->classes.find("ipgm")->second.atom.asOpcode())
				ram_objects[i]=mem->buildInstantiatedProgram(code_segment.objects[i]);
			else	if(code_segment.objects[i]->code[0].getDescriptor()	&	Atom::MARKER)
				ram_objects[i]=mem->buildMarker(code_segment.objects[i]);
			else
				ram_objects[i]=mem->buildObject(code_segment.objects[i]);
		}
		//	Translate indices into pointers
		for(i=0;i<relocation_segment.entries.size();++i){	//	for each allocated object, write its address in the reference set of the objects or views that reference it

			r_code::Object	*referenced_object=ram_objects[i];
			RelocationSegment::Entry	e=relocation_segment.entries[i];
			for(uint32	j=0;j<e.pointer_indexes.size();++j){

				RelocationSegment::PointerIndex	p=e.pointer_indexes[j];
				r_code::Object	*referencing_object=ram_objects[p.object_index];
				switch(p.view_index){
				case	-1:
					referencing_object->reference_set[p.pointer_index]=referenced_object;
					break;
				case	-2:
					referencing_object->marker_set[p.pointer_index]=referenced_object;
					break;
				default:
					referencing_object->view_set[p.view_index]->reference_set[p.pointer_index]=referenced_object;
					break;
				}
			}
		}
		if(noMem)
			delete	mem;
	}

	////////////////////////////////////////////////////////////////

	Image::Image(){
	}

	Image::~Image(){
	}
}