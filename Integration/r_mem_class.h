#ifndef	r_mem_class_h
#define	r_mem_class_h

#include	"mbrane.h"
#include	"image.h"

using	namespace	mBrane;
using	namespace	mBrane::sdk;

using	namespace	r_code;

class	TestPayload:
public	Message<TestPayload,Memory>{
public:
	uint32	data;
};

class	ImageCore{
protected:
	uint32	_def_size;
	uint32	_map_size;
	uint32	_code_size;
	uint32	_reloc_size;
public:
	ImageCore():_def_size(0),_map_size(0),_code_size(0),_reloc_size(0){}
	uint32	def_size()		const{	return	_def_size;	}
	uint32	map_size()		const{	return	_map_size;	}
	uint32	code_size()		const{	return	_code_size;	}
	uint32	reloc_size()	const{	return	_reloc_size;	}
};

template<class	U>	class	_ImageMessage:
public	CompoundMessage<U,Memory,ImageCore,word32>{
public:
	_ImageMessage(){}
	_ImageMessage(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size){
		_def_size=def_size;
		_map_size=map_size;
		_code_size=code_size;
		_reloc_size=reloc_size;
	}
};

class	ImageMessage:
public	Image<_ImageMessage<ImageMessage> >{
public:
	ImageMessage(){}
};


#endif