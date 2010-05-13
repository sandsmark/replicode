#ifndef	image_impl_h
#define	image_impl_h

#include	"types.h"


using	namespace	core;

class	ImageImpl{
private:
	word32	*_data;	//	[def segment|object map|code segment|reloc segment]
	uint32	_def_size;
	uint32	_map_size;
	uint32	_code_size;
	uint32	_reloc_size;
protected:
	uint32	def_size()		const;
	uint32	map_size()		const;
	uint32	code_size()		const;
	uint32	reloc_size()	const;
	word32	*data();
public:
	void	*operator	new(size_t,uint32	data_size);
	void	operator delete(void	*o);
	ImageImpl(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size);
	~ImageImpl();
};


#endif