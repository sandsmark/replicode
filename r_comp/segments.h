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
	//	Bothe images are equivalent, the latter being easier to work with (uses vectors instead of a contiguous structure, that is r_code::Image::data).
	//	All read(word32*)/write(word32) functions defined in the classes below perfom read/write operations in an r_code::Image::data.

	class	dll_export	DefinitionSegment{
	public:
		DefinitionSegment();

		UNORDERED_MAP<std::string,Class>	classes;
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
	public:
		DefinitionSegment	*definition_segment;
		ObjectMap			object_map;
		CodeSegment			code_segment;
		RelocationSegment	relocation_segment;

		Image();
		Image(DefinitionSegment	*definition_segment);

		void	addObject(SysObject	*object);
		
		void	write(r_code::Image	*image);
		void	read(r_code::Image	*image);
	};
}


#endif