//	image_impl.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include	"image_impl.h"


namespace	r_code{

	void	*ImageImpl::operator	new(size_t	s,uint32	data_size){

		return	::operator	new(s);
	}

	void	ImageImpl::operator delete(void	*o){

		::operator	delete(o);
	}

	ImageImpl::ImageImpl(uint64	timestamp,uint32	map_size,uint32	code_size):_timestamp(timestamp),_map_size(map_size),_code_size(code_size){

		_data=new	word32[_map_size+_code_size];
	}

	ImageImpl::~ImageImpl(){

		delete[]	_data;
	}

	uint64	ImageImpl::get_timestamp()	const{

		return	_timestamp;
	}

	uint32	ImageImpl::map_size()	const{

		return	_map_size;
	}

	uint32	ImageImpl::code_size()	const{

		return	_code_size;
	}

	word32	*ImageImpl::data()	const{

		return	_data;
	}

	word32	&ImageImpl::data(uint32	i){

		return	_data[i];
	}

	word32	&ImageImpl::data(uint32	i)	const{

		return	_data[i];
	}
}