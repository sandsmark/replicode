#include	"r_mem_class.h"


__Payload	*CodePayload::getPtr(uint16	i)	const{
	
	return	((RObject	*)(P<RObject>	*)&data(code_size+i))->get_payload();
}