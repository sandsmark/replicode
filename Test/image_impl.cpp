#include	"image_impl.h"


void	*ImageImpl::operator	new(size_t	s,uint32	data_size){

	return	::operator	new(s);
}

void	ImageImpl::operator delete(void	*o){

	::operator	delete(o);
}

ImageImpl::ImageImpl(uint32	map_size,uint32	code_size,uint32	reloc_size):_map_size(map_size),_code_size(code_size),_reloc_size(reloc_size){

	_data=new	word32[_map_size+_code_size+_reloc_size];
}

ImageImpl::~ImageImpl(){

	delete[]	_data;
}

uint32	ImageImpl::map_size()	const{

	return	_map_size;
}

uint32	ImageImpl::code_size()	const{

	return	_code_size;
}

uint32	ImageImpl::reloc_size()	const{

	return	_reloc_size;
}

word32	*ImageImpl::data()	const{

	return	_data;
}