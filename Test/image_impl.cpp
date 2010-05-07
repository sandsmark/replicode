#include	"image_impl.h"


void	*ImageImpl::operator	new(size_t,uint32	data_size){

	return	::new(size_t);
}

ImageImpl::ImageImpl(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size):_def_size(def_size),_map_size(map_size),_code_size(code_size),_reloc_size(reloc_size){

	_data=new	word32[_def_size+_map_size+_code_size+_reloc_size];
}

ImageImpl::~ImageImpl(){

	delete[]	_data;
}

uint32	ImageImpl::def_size()	const{

	return	_def_size;
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

word32	*ImageImpl::data(){

	return	_data;
}