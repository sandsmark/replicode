#include	"r_mem_class.h"


__Payload	*CodePayload::getPtr(uint16	i)	const{
	
	return	((RObject	*)&data(code_size+i))->get_payload();
}
