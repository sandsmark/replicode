#include	"r_mem_class.h"


__Payload	*CodePayload::getPtr(uint16	i)	const{
	
	return	((RObject	*)&data(code_size+i))->get_payload();
}

void	CodePayload::setPtr(uint16	i,__Payload	*p){

	*((P<CodePayload>	*)&data(code_size+i))=(CodePayload	*)p;
}
