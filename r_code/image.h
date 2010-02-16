#ifndef image_h
#define	image_h

#include	"types.h"

#include	<fstream>


using	namespace	std;

namespace	r_comp{
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
	class	dll_export	Image{
	friend	class r_comp::Image;
	private:
		uint32	def_size;
		uint32	map_size;
		uint32	code_size;
		uint32	reloc_size;
		word32	*data;	//	[def segment|object map|code segment|reloc segment]
		word32	*getDefSegment()	const;
		uint32	getDefSegmentSize()	const;
	public:
		Image();
		~Image();

		uint32	getSize()				const;	//	size of data in word32
		uint32	getObjectCount()		const;
		word32	*getObject(uint32	i)	const;	//	points to the code size of the object; the first atom is at getObject()+2
		word32	*getCodeSegment()		const;	//	equals getObject(0)
		uint32	getCodeSegmentSize()	const;
		word32	*getRelocSegment()		const;
		uint32	getRelocSegmentSize()	const;
		word32	*getRelocEntry(uint32	index)	const;	//	retreives the entry at the specified index from the reloc segment
														//	first word: index of the object in the code segment, followed by number of indexes in the list; followed by a list of indexes
														//	return NULL if entry not found
		void	read(ifstream &stream);
		void	write(ofstream &stream);
		void	trace()	const;
	};
}


#endif
