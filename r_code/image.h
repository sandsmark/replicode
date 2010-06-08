//	image.h
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

#ifndef r_code_image_h
#define	r_code_image_h

#include	"types.h"

#include	<fstream>


using	namespace	std;
using	namespace	core;

namespace	r_comp{
	class	ClassImage;
	class	CodeImage;
	class	Image;
}

namespace	r_code{

	//	An image contains the following:
	//		- sizes of what follows
	//		- def segment: list of class, operators and function definitions, and object names
	//		- object map: list of indexes (4 bytes) of objects in the code segment
	//		- code segment: list of objects
	//			- object:
	//				- size of the code (number of atoms)
	//				- size of the reference set (number of pointers)
	//				- size of the marker set (number of pointers)
	//				- size of the view set (number of views)
	//				- code: indexes in internal pointers are relative to the beginning of the object, indexes in reference pointers are relative to the beginning of the reference set
	//				- reference set:
	//					- number of pointers
	//					- pointers to the relocation segment, i.e. indexes of relocation entries
	//				- marker set:
	//					- number of pointers
	//					- pointers to the relocation segment, i.e. indexes of relocation entries
	//				- view set: list of views 
	//					- view:
	//						- size of the code (number of atoms)
	//						- size of the reference set (number of pointers)
	//						- list of atoms
	//						- reference set:
	//							- pointers to the relocation segment, i.e. indexes of relocation entries
	//		- relocation segment:
	//			- number of entries (same number as in object map)
	//			- list of entries
	//			the index of an entry in the relocation segment is the index of the referenced object (RO) in the code segment
	//				- entry:
	//					- the number of references to RO
	//					- list of references to RO
	//						- reference:
	//							- index of an object O referencing RO (use getObject())
	//							- index of a view of O (-1 means that the object itself references RO)
	//							- index of the pointer in the reference set of the object or view
	//
	//	RAM layout of Image::data [sizes in word32]:
	//
	//		data[0]:										first word32 of def segment [1]
	//		...												...
	//		data[def_size-1]:								last word32 of def segment [1]
	//		data[def_size]:									index of first object in data (i.e. def_size+map_size) [1]
	//		...												...
	//		data[def_size+map_size-1]:						index of last object in data [1]
	//		data[def_size+map_size]:						first word32 of code segment [1]
	//		...												...
	//		data[def_size+map_size+code_size-1]:			last word32 of code segment [1]
	//		data[def_size+map_size+code_size]:				first word32 of	reloc segment [1]
	//		...												...
	//		data[def_size+map_size+code_size+reloc_size-1]:	last word32 of reloc segment [1]
	//
	//	Def segment is private to the compiler: the only requirement is to keep def segment aligned on 4 bytes.
	//
	//	I is the implementation class; prototype:
	//	class	ImageImpl{
	//	protected:
	//		uint32	def_size()		const;
	//		uint32	map_size()		const;
	//		uint32	code_size()		const;
	//		uint32	reloc_size()	const;
	//		word32	*data();	//	[def segment|object map|code segment|reloc segment]
	//	public:
	//		void	*operator	new(size_t,uint32	data_size);
	//		ImageImpl(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size);
	//		~ImageImpl();
	//	};

	template<class	I>	class	Image:
	public	I{
	friend	class r_comp::CodeImage;
	friend	class r_comp::ClassImage;
	friend	class r_comp::Image;
	private:
		word32	*getDefSegment();
		uint32	getDefSegmentSize()	const;
	public:
		static	Image<I>	*Build(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size);
		//	file IO
		static	Image<I>	*Read(ifstream &stream);
		static	void		Write(Image<I>	*image,ofstream &stream);

		Image();
		~Image();

		uint32	getSize()	const;				//	size of data in word32
		uint32	getObjectCount()	const;
		word32	*getObject(uint32	i);			//	points to the code size of the object; the first atom is at getObject()+2
		word32	*getCodeSegment();				//	equals getObject(0)
		uint32	getCodeSegmentSize()	const;
		word32	*getRelocSegment();
		uint32	getRelocSegmentSize()	const;
		word32	*getRelocEntry(uint32	index);	//	retreives the entry at the specified index from the reloc segment
												//	first word: index of the object in the code segment, followed by number of indexes in the list; followed by a list of indexes
												//	return NULL if entry not found
		void	trace()	const;
	};

	//	utilities
	uint32	dll_export	GetSize(const	std::string	&s);	//	returns the number of word32 needed to encode the string
	void	dll_export	Write(word32	*data,const	std::string	&s);
	void	dll_export	Read(word32	*data,std::string	&s);
}


#include	"image.tpl.cpp"


#endif
