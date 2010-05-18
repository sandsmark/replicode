#ifndef	r_code_h
#define	r_code_utils_h

#include	"atom.h"


namespace	r_code{

	class	dll_export	Timestamp{
	public:
		static	uint64	Get(Atom	*iptr);
		static	void	Set(Atom	*iptr,uint64	t);
	};
}


#endif