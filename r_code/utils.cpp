#include	"utils.h"


namespace	r_code{

	uint64	Timestamp::Get(Atom	*iptr){

		uint32	i=iptr->asIndex();
		return	iptr[i+1].atom<<32	|	iptr[i+2].atom;
	}

	void	Timestamp::Set(Atom	*iptr,uint64	t){

		uint32	i=iptr->asIndex();
		iptr[i+1].atom=t<<32;
		iptr[i+2].atom=t	&	0x00000000FFFFFFFF;
	}
}