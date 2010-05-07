//	out_stream.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

#ifndef	out_stream_h
#define	out_stream_h

#include	<iostream>

#include	"vector.h"


using	namespace	r_code;

namespace	r_comp{

	//	Allows inserting data at a specified index and right shifting the current content; 
	//	ex: labels and variables, i.e. when iptrs are discovered and these hold indexes are < read_index and do not point to variables 
	class	OutStream{
	public:
		r_code::vector<uint16>	code_indexes_to_stream_indexes;
		uint16	code_index;
		r_code::vector<std::streampos>	positions;
		OutStream(std::ostringstream	*s):stream(s){}
		std::ostringstream	*stream;
		template<typename	T>	OutStream	&push(const	T	&t,uint16	code_index){
			positions.push_back(stream->tellp());
			code_indexes_to_stream_indexes[code_index]=positions.size()-1;
			return	*this<<t;
		}
		OutStream	&push(){	//	to keep adding entries in v without outputing anything (e.g. for wildcards after ::)
			std::streampos	p;
			positions.push_back(p);
			return	*this;
		}
		template<typename	T>	OutStream	&operator	<<(const	T	&t){
			*stream<<t;
			return	*this;
		}
		template<typename	T>	OutStream	&insert(uint32	index,const	T	&t){	//	inserts before code_indexes_to_stream_indexes[index]
			uint16	stream_index=code_indexes_to_stream_indexes[index];
			stream->seekp(positions[stream_index]);
			std::string	s=stream->str().substr(positions[stream_index]);
			*stream<<t;
			std::streamoff	offset=stream->tellp()-positions[stream_index];
			*stream<<s;
			for(uint16	i=stream_index+1;i<positions.size();++i)	//	right-shift
				positions[i]+=offset;
			return	*this;
		}
	};

	class	NoStream:
	public	std::ostream{
	public:
		NoStream():std::ostream(NULL){}
		template<typename	T>	NoStream&	operator	<<(T	&t){
			return	*this;
		}
	};

	class	CompilerOutput:
	public	std::ostream{
	public:
		CompilerOutput():std::ostream(NULL){}
		template<typename	T>	std::ostream&	operator	<<(T	&t){
			if(1) // njt: was if (Output); HACK for linux compatibility
				return	std::cout<<t;
			return	*this;
		}
	};
}


#endif
