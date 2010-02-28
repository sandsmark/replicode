#ifndef	segments_h
#define	segments_h

#include	"../r_code/object.h"
#include	"class.h"


using	namespace	r_code;

namespace	r_comp{

	class	Reference{
	public:
		Reference();
		Reference(const	uint16	i,const	Class	&c);
		uint16	index;
		Class	_class;
	};

	//	All classes below map components of r_code::Image into r_comp::Image.
	//	Both images are equivalent, the latter being easier to work with (uses vectors instead of a contiguous structure, that is r_code::Image::data).
	//	All read(word32*,uint32)/write(word32*) functions defined in the classes below perfom read/write operations in an r_code::Image::data.

	class	dll_export	DefinitionSegment{
	private:
		uint32	getClassArraySize();
		uint32	getClassesSize();
		uint32	getSysClassesSize();
		uint32	getClassNamesSize();
		uint32	getOperatorNamesSize();
		uint32	getFunctionNamesSize();
	public:
		DefinitionSegment();

		UNORDERED_MAP<std::string,Class>	classes;	//	non-sys classes, operators and device functions
		UNORDERED_MAP<std::string,Class>	sys_classes;

		r_code::vector<std::string>	class_names;		//	classes and sys-classes; does not include set classes
		r_code::vector<std::string>	operator_names;
		r_code::vector<std::string>	function_names;
		r_code::vector<Class>		classes_by_opcodes;	//	classes indexed by opcodes; used to retrieve member names; registers all classes (incl. set classes)

		Class	*getClass(std::string	&class_name);
		Class	*getClass(uint16	opcode);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	getSize();
	};

	class	dll_export	ObjectMap{
	public:
		r_code::vector<uint32>	objects;

		void	shift(uint32	offset);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	getSize()	const;
	};

	class	dll_export	CodeSegment{
	public:
		r_code::vector<SysObject	*>	objects;
		
		~CodeSegment();
		
		void	write(word32	*data);
		void	read(word32		*data,uint32	object_count);
		uint32	getSize();
	};

	class	dll_export	RelocationSegment{
	public:
		class	dll_export	PointerIndex{
		public:
			uint32	object_index;	//	index of a referencing object in CodeSegment::objects
			int32	view_index;		//	index of a view in the object's view set
			uint32	pointer_index;	//	index of a pointer in CodeSegment::objects[object_index]
			PointerIndex();
			PointerIndex(uint32	object_index,int32	view_index,uint32	pointer_index);
		};
		class	dll_export	Entry{
		public:
			r_code::vector<PointerIndex>	pointer_indexes;	//	indexes of the pointers referencing the object
			void	write(word32	*data);
			void	read(word32		*data);
			uint32	getSize()	const;
		};

		r_code::vector<Entry>	entries;

		void	addObjectReference(uint32	referenced_object_index,
									uint32	referencing_object_index,
									uint32	reference_pointer_index);
		void	addViewReference(uint32	referenced_object_index,
									uint32	referencing_object_index,
									int32	referencing_view_index,
									uint32	reference_pointer_index);
		void	addMarkerReference(uint32	referenced_object_index,
									uint32	referencing_object_index,
									uint32	reference_pointer_index);

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize();
	};

	class	dll_export	Image{
	private:
		uint32	map_offset;
		UNORDERED_MAP<Object	*,uint32>	ptrs_to_indices;	//	used for >> in memory
		void	buildReferences(SysObject	*sys_object,Object	*object,uint32	object_index);
	public:
		DefinitionSegment	definition_segment;
		ObjectMap			object_map;
		CodeSegment			code_segment;
		RelocationSegment	relocation_segment;

		Image();
		~Image();

		void	addObject(SysObject	*object);
		
		Image	&operator	>>	(r_code::Image	*image);
		Image	&operator	<<	(r_code::Image	*image);

		Image	&operator	>>	(r_code::vector<Object	*>	&ram_objects);
		Image	&operator	<<	(r_code::vector<Object	*>	&ram_objects);

		Image	&operator	<<	(Object	*object);

		void	removeObjects();
	};
}


#endif