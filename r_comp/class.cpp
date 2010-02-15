#include	"class.h"
#include	"segments.h"


namespace	r_comp{

	bool	Class::has_offset()	const{

		switch(atom.getDescriptor()){
		case	Atom::OBJECT:
		case	Atom::MARKER:	return	true;
		default:				return	false;
		}
	}

	Class::Class(ReturnType	t):type(t){
	}
		
	Class::Class(Atom							atom,
				std::string						str_opcode,
				std::vector<StructureMember>	r,
				ReturnType						t):atom(atom),
												str_opcode(str_opcode),
												things_to_read(r),
												type(t),
												use_as(StructureMember::CLASS){
	}
	
	bool	Class::is_pattern(DefinitionSegment	*segment)	const{
		
		return	segment->classes.find("ptn")->second.atom==atom;
	}
		
	bool	Class::get_member_index(DefinitionSegment	*segment,std::string	&name,uint16	&index,Class	*&p)	const{
			
		for(uint16	i=0;i<things_to_read.size();++i)
			if(things_to_read[i].name==name){

				index=(has_offset()?i+1:i);	//	in expressions the lead r-atom is at 0; in objects, members start at 1
				if(things_to_read[i].used_as_expression())	//	the class is: [::a-class]
					p=NULL;
				else
					p=things_to_read[i].get_class(segment);
				return	true;
			}
		return	false;
	}
		
	std::string	Class::get_member_name(uint32	index){
		
		return	things_to_read[has_offset()?index-1:index].name;
	}

	ReturnType	Class::get_member_type(const	uint16	index){
		
		return	things_to_read[has_offset()?index-1:index].get_return_type();
	}

	Class	*Class::get_member_class(DefinitionSegment	*segment,const	std::string	&name){

		for(uint16	i=0;i<things_to_read.size();++i)
			if(things_to_read[i].name==name)
				return	things_to_read[i].get_class(segment);
		return	NULL;
	}

	void	Class::write(word32	*storage){

	}

	void	Class::read(word32	*storage){
		
	}
}