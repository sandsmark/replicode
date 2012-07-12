//	utils.h
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

#ifndef	r_code_utils_h
#define	r_code_utils_h

#include	"atom.h"
#include	"../../CoreLibrary/trunk/CoreLibrary/base.h"


namespace	r_code{

	// For use in STL containers.
	template<class	C>	class	PHash{
	public:
		size_t	operator	()(P<C>	c)	const{
			return	(size_t)(C	*)c;
		}
	};

	class	Code;

	class	dll_export	Utils{
	private:
		static	uint64	TimeReference;	// starting time.
		static	uint32	BasePeriod;
		static	float32	FloatTolerance;
		static	uint32	TimeTolerance;
	public:
		static	uint64	GetTimeReference();
		static	uint32	GetBasePeriod();
		static	uint32	GetFloatTolerance();
		static	uint32	GetTimeTolerance();
		static	void	SetReferenceValues(uint64	time_reference,uint32	base_period,float32	float_tolerance,uint32	time_tolerance);

		static	bool	Equal(float32	l,float32	r);
		static	bool	Synchronous(uint64	l,uint64	r);

		static	uint64	GetTimestamp(const	Atom	*iptr);
		static	void	SetTimestamp(Atom	*iptr,uint64	t);
		static	void	SetTimestamp(Code	*object,uint16	index,uint64	t);	// allocates atoms.

		static	const	uint64	MaxTime=0xFFFFFFFFFFFFFFFF;

		template<class	O>	static	uint64	GetTimestamp(const	O	*object,uint16	index){

			uint16	t_index=object->code(index).asIndex();
			uint64	high=object->code(t_index+1).atom;
			return	high<<32	|	object->code(t_index+2).atom;
		}

		template<class	O>	static	void	SetTimestamp(O	*object,uint16	index,uint64	t){

			uint16	t_index=object->code(index).asIndex();
			object->code(t_index)=Atom::Timestamp();
			object->code(t_index+1).atom=t>>32;
			object->code(t_index+2).atom=t	&	0x00000000FFFFFFFF;
		}

		static	std::string	GetString(const	Atom	*iptr);
		static	void	SetString(Atom	*iptr,const	std::string	&s);

		template<class	O>	static	std::string	GetString(const	O	*object,uint16	index){

			uint16	s_index=object->code(index).asIndex();
			std::string	s;
			char	buffer[255];
			uint8	char_count=(object->code(s_index).atom	&	0x000000FF);
			memcpy(buffer,&object->code(s_index+1),char_count);
			buffer[char_count]=0;
			s+=buffer;
			return	s;
		}

		template<class	O>	static	void	SetString(O	*object,uint16	index,const	std::string	&s){

			uint16	s_index=object->code(index).asIndex();
			uint8	l=(uint8)s.length();
			object->code(s_index)=Atom::String(l);
			uint32	_st=0;
			int8	shift=0;
			for(uint8	i=0;i<l;++i){
				
				_st|=s[i]<<shift;
				shift+=8;
				if(shift==32){

					object->code(++s_index)=_st;
					_st=0;
					shift=0;
				}
			}
			if(l%4)
				object->code(++s_index)=_st;
		}

		static	int32	GetResilience(uint64	now,uint64	time_to_live,uint64	upr);	// ttl: us, upr: us.
		static	int32	GetResilience(float32	resilience,float32	origin_upr,float32	destination_upr);	// express the res in destination group, given the res in origin group.
	};
}


#endif