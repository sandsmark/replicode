#include	"operator.h"


namespace	r_code{

	OperatorRegister	OperatorRegister::Singleton;

	OperatorRegister	&OperatorRegister::Get(){

		return	Singleton;
	}

	OperatorRegister::OperatorRegister(){
	}

	OperatorRegister::~OperatorRegister(){
	}

	OperatorRegister::Operator	&OperatorRegister::operator [](uint16 opcode){

		return	operators[opcode];
	}
}