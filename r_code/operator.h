#ifndef	operator_h
#define	operator_h

#include	"execution_context.h"


namespace	r_code{

	class	OperatorRegister{
	public:
		typedef	void	(*Operator)(ExecutionContext	&context);
	private:
		static	OperatorRegister	Singleton;
		Operator	operators[256];
		OperatorRegister();
		~OperatorRegister();
	public:
		static	OperatorRegister	&Get();
		Operator	&operator [](uint16 opcode);
	};
}


#endif