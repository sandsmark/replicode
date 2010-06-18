#ifndef	image_impl_h
#define	image_impl_h

#include	"types.h"


using	namespace	core;

class	ImageImpl{
private:
	word32	*_data;	//	[object map|code segment|reloc segment]
	uint32	_map_size;
	uint32	_code_size;
	uint32	_reloc_size;
protected:
	uint32	map_size()		const;
	uint32	code_size()		const;
	uint32	reloc_size()	const;
	word32	*data()			const;
public:
	void	*operator	new(size_t,uint32	data_size);
	void	operator delete(void	*o);
	ImageImpl(uint32	map_size,uint32	code_size,uint32	reloc_size);
	~ImageImpl();
};


#endif