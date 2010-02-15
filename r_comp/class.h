#ifndef	class_h
#define	class_h

#include	"structure_member.h"


namespace	r_comp{

	class	Compiler;

	class	Class{
	private:
		bool	has_offset()	const;
	public:
		Class(ReturnType	t=ANY);
		Class(Atom							atom,
			std::string						str_opcode,
			std::vector<StructureMember>	r,
			ReturnType						t=ANY);
		bool	is_pattern(DefinitionSegment	*segment)	const;
		bool	get_member_index(DefinitionSegment	*segment,std::string	&name,uint16	&index,Class	*&p)	const;
		std::string	get_member_name(uint32	index);	//	for decompilation
		ReturnType	get_member_type(const	uint16	index);
		Class		*get_member_class(DefinitionSegment	*segment,const	std::string	&name);
		Atom							atom;
		std::string						str_opcode;			//	unused for anything but objects, markers and operators
		std::vector<StructureMember>	things_to_read;
		ReturnType						type;				//	ANY for non-operators
		StructureMember::Iteration		use_as;

		void	write(word32	*storage);
		void	read(word32		*storage);
	};
}


#endif