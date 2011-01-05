//	utils.cpp
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

#include	"utils.h"


namespace	r_code{

	uint64	Utils::GetTimestamp(const	Atom	*iptr){

		uint64	high=iptr[1].atom;
		return	high<<32	|	iptr[2].atom;
	}

	void	Utils::SetTimestamp(Atom	*iptr,uint64	t){

		iptr[0]=Atom::Timestamp();
		iptr[1].atom=t>>32;
		iptr[2].atom=t	&	0x00000000FFFFFFFF;
	}

	std::string	Utils::GetString(const	Atom	*iptr){

		std::string	s;
		char	buffer[255];
		uint8	char_count=(iptr[0].atom	&	0x000000FF);
		memcpy(buffer,iptr+1,char_count);
		buffer[char_count]=0;
		s+=buffer;
		return	s;
	}

	void	Utils::SetString(Atom	*iptr,const	std::string	&s){

		uint8	l=(uint8)s.length();
		uint8	index=0;
		iptr[index]=Atom::String(l);
		uint32	_st=0;
		int8	shift=0;
		for(uint8	i=0;i<l;++i){
			
			_st|=s[i]<<shift;
			shift+=8;
			if(shift==32){

				iptr[++index]=_st;
				_st=0;
				shift=0;
			}
		}
		if(l%4)
			iptr[++index]=_st;
	}
}