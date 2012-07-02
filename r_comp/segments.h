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

	// All classes below map components of r_code::Image into r_comp::Image.
	// Both images are equivalent, the latter being easier to work with (uses vectors instead of a contiguous structure, that is r_code::Image::data).
	// All read(word32*,uint32)/write(word32*) functions defined in the classes below perfom read/write operations in an r_code::Image::data.

	class	dll_export	Metadata{
	private:
		uint32	get_class_array_size();
		uint32	get_classes_size();
		uint32	get_sys_classes_size();
		uint32	get_class_names_size();
		uint32	get_operator_names_size();
		uint32	get_function_names_size();
	public:
		Metadata();

		UNORDERED_MAP<std::string,Class>	classes;	// non-sys classes, operators and device functions.
		UNORDERED_MAP<std::string,Class>	sys_classes;

		r_code::vector<std::string>	class_names;		// classes and sys-classes; does not include set classes.
		r_code::vector<std::string>	operator_names;
		r_code::vector<std::string>	function_names;
		r_code::vector<Class>		classes_by_opcodes;	// classes indexed by opcodes; used to retrieve member names; registers all classes (incl. set classes).

		Class	*get_class(std::string	&class_name);
		Class	*get_class(uint16	opcode);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	get_size();
	};

	class	dll_export	ObjectMap{
	public:
		r_code::vector<uint16>	objects;

		void	shift(uint16	offset);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	get_size()	const;
	};

	class	dll_export	CodeSegment{
	public:
		r_code::vector<SysObject	*>	objects;
		
		~CodeSegment();
		
		void	write(word32	*data);
		void	read(word32		*data,uint16	object_count);
		uint32	get_size();
	};

	class	dll_export	ObjectNames{
	public:
		UNORDERED_MAP<uint32,std::string>	symbols;	// indexed by objects' OIDs.

		~ObjectNames();

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	get_size();
	};

	class	dll_export	Image{
	private:
		uint32	map_offset;
		UNORDERED_MAP<r_code::Code	*,uint16>	ptrs_to_indices;	// used for injection in memory.

		void		add_object(r_code::Code	*object);
		SysObject	*add_object(Code	*object,std::vector<SysObject	*>	&imported_objects);
		uint32		get_reference_count(Code	*object);
		void		build_references();
		void		build_references(SysObject	*sys_object,r_code::Code	*object);
		void		unpack_objects(r_code::vector<Code	*>	&ram_objects);
	public:
		ObjectMap	object_map;
		CodeSegment	code_segment;
		ObjectNames	object_names;

		uint64	timestamp;

		Image();
		~Image();

		void	add_sys_object(SysObject	*object,std::string	name);	// called by the compiler.
		void	add_sys_object(SysObject	*object);					// called by add_object().

		void	get_objects(Mem	*mem,r_code::vector<r_code::Code	*>	&ram_objects);
		template<class	O>	void	get_objects(r_code::vector<Code	*>	&ram_objects){

			for(uint32	i=0;i<code_segment.objects.size();++i){

				uint16	opcode=code_segment.objects[i]->code[0].asOpcode();
				ram_objects[i]=new	O(code_segment.objects[i]);
			}
			unpack_objects(ram_objects);
		}

		void	add_objects(std::list<P<r_code::Code> >	&objects);												// called by the rMem.
		void	add_objects(std::list<P<r_code::Code> >	&objects,std::vector<SysObject	*>	&imported_objects);	// called by any r_exec code for decompiling on the fly.

		template<class	I>	I	*serialize(){

			I	*image=(I	*)I::Build(timestamp,object_map.get_size(),code_segment.get_size(),object_names.get_size());

			object_map.shift(image->map_size());
			object_map.write(image->data());
			code_segment.write(image->data()+image->map_size());
			object_names.write(image->data()+image->map_size()+image->code_size());

			return	image;
		}

		template<class	I>	void	load(I	*image){

			timestamp=image->timestamp();
			object_map.read(image->data(),image->map_size());
			code_segment.read(image->data()+image->map_size(),image->map_size());
			object_names.read(image->data()+image->map_size()+image->code_size());
		}
	};
}


#endif