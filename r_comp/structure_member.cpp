#include	"structure_member.h"
#include	"compiler.h"


namespace	r_comp{

	StructureMember::StructureMember(_Read		r,
									std::string	m,
									std::string	p,
									Iteration	i):_read(r),
													name(m),
													_class(p),
													iteration(i){

				if(_read==&Compiler::read_number)		type=NUMBER;
		else	if(_read==&Compiler::read_boolean)		type=BOOLEAN;
		else	if(_read==&Compiler::read_string)		type=STRING;
		else	if(_read==&Compiler::read_node)			type=NODE_ID;
		else	if(_read==&Compiler::read_device)		type=DEVICE_ID;
		else	if(_read==&Compiler::read_function)		type=FUNCTION_ID;
		else	if(_read==&Compiler::read_expression)	type=ANY;
		else	if(_read==&Compiler::read_set)			type=(ReturnType)SET;
	}
		
	Class	*StructureMember::get_class(DefinitionSegment	*segment)	const{
		
		return	_class==""?NULL:&segment->classes.find(_class)->second;
	}
	
	ReturnType	StructureMember::get_return_type()	const{
		
		return	type;
	}

	bool	StructureMember::used_as_expression()	const{
		
		return	iteration==EXPRESSION;
	}

	StructureMember::Iteration	StructureMember::getIteration()	const{

		return	iteration;
	}

	_Read	StructureMember::read()	const{

		return	_read;
	}
}