//	segments.h
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

#ifndef	segments_h
#define	segments_h

#include	"../r_code/object.h"
#include	"class.h"


using	namespace	r_code;

namespace	r_comp{

	class	Reference{
	public:
		Reference();
		Reference(const	uint16	i,const	Class	&c,const	Class	&cc);
		uint16	index;
		Class	_class;
		Class	cast_class;
	};

	//	All classes below map components of r_code::Image into r_comp::Image.
	//	Both images are equivalent, the latter being easier to work with (uses vectors instead of a contiguous structure, that is r_code::Image::data).
	//	All read(word32*,uint32)/write(word32*) functions defined in the classes below perfom read/write operations in an r_code::Image::data.

	class	dll_export	Metadata{
	private:
		uint32	getClassArraySize();
		uint32	getClassesSize();
		uint32	getSysClassesSize();
		uint32	getClassNamesSize();
		uint32	getOperatorNamesSize();
		uint32	getFunctionNamesSize();
	public:
		Metadata();

		UNORDERED_MAP<std::string,Class>	classes;	//	non-sys classes, operators and device functions.
		UNORDERED_MAP<std::string,Class>	sys_classes;

		r_code::vector<std::string>	class_names;		//	classes and sys-classes; does not include set classes.
		r_code::vector<std::string>	operator_names;
		r_code::vector<std::string>	function_names;
		r_code::vector<Class>		classes_by_opcodes;	//	classes indexed by opcodes; used to retrieve member names; registers all classes (incl. set classes).

		Class	*getClass(std::string	&class_name);
		Class	*getClass(uint16	opcode);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	getSize();
	};

	class	dll_export	ObjectMap{
	public:
		r_code::vector<uint16>	objects;

		void	shift(uint16	offset);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	getSize()	const;
	};

	class	dll_export	CodeSegment{
	public:
		r_code::vector<SysObject	*>	objects;
		
		~CodeSegment();
		
		void	write(word32	*data);
		void	read(word32		*data,uint16	object_count);
		uint32	getSize();
	};

	class	dll_export	Image{
	private:
		uint32	map_offset;
		UNORDERED_MAP<r_code::Code	*,uint16>	ptrs_to_indices;	//	used for >> in memory.
		void	buildReferences(SysObject	*sys_object,r_code::Code	*object,uint16	object_index);
		void	unpackObjects(r_code::vector<Code	*>	&ram_objects);
	public:
		ObjectMap	object_map;
		CodeSegment	code_segment;

		uint64	timestamp;

		Image();
		~Image();

		void	addObject(SysObject	*object);

		void	getObjects(Mem	*mem,r_code::vector<r_code::Code	*>	&ram_objects);
		template<class	O>	void	getObjects(r_code::vector<Code	*>	&ram_objects){

			for(uint32	i=0;i<code_segment.objects.size();++i){

				uint16	opcode=code_segment.objects[i]->code[0].asOpcode();
				ram_objects[i]=new	O(code_segment.objects[i]);
			}
			unpackObjects(ram_objects);
		}

		Image	&operator	<<	(r_code::vector<r_code::Code	*>	&ram_objects);
		Image	&operator	<<	(r_code::Code	*object);

		template<class	I>	I	*serialize(){

			I	*image=(I	*)I::Build(timestamp,object_map.getSize(),code_segment.getSize());

			object_map.shift(image->map_size());
			object_map.write(image->data());
			code_segment.write(image->data()+image->map_size());

			return	image;
		}

		template<class	I>	void	load(I	*image){

			timestamp=image->get_timestamp();
			object_map.read(image->data(),image->map_size());
			code_segment.read(image->data()+image->map_size(),image->map_size());
		}
	};
}


#endif