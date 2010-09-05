//	image_impl.h
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

#ifndef	image_impl_h
#define	image_impl_h

#include	"types.h"


using	namespace	core;

namespace	r_code{

	class	dll_export	ImageImpl{
	private:
		word32	*_data;	//	[object map|code segment|reloc segment]
		uint64	_timestamp;
		uint32	_map_size;
		uint32	_code_size;
	protected:
		uint64	get_timestamp()	const;
		uint32	map_size()		const;
		uint32	code_size()		const;
		word32	*data()			const;
		word32	&data(uint32	i);
		word32	&data(uint32	i)	const;
	public:
		void	*operator	new(size_t,uint32	data_size);
		void	operator delete(void	*o);
		ImageImpl(uint64	timestamp,uint32	map_size,uint32	code_size);
		~ImageImpl();
	};
}


#endif