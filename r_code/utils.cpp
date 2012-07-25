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
#include	"object.h"

#include	<math.h>


namespace	r_code{

	uint64	Utils::TimeReference=0;
	uint32	Utils::BasePeriod=0;
	float32	Utils::FloatTolerance=0;
	uint32	Utils::TimeTolerance=0;

	uint64	Utils::GetTimeReference(){	return	TimeReference;	}
	uint32	Utils::GetBasePeriod(){	return	BasePeriod;	}
	uint32	Utils::GetFloatTolerance(){	return	FloatTolerance;	}
	uint32	Utils::GetTimeTolerance(){	return	TimeTolerance;	}

	void	Utils::SetReferenceValues(uint32	base_period,float32	float_tolerance,uint32	time_tolerance){

		BasePeriod=base_period;
		FloatTolerance=float_tolerance;
		TimeTolerance=time_tolerance;
	}

	void	Utils::SetTimeReference(uint64	time_reference){

		TimeReference=time_reference;
	}

	bool	Utils::Equal(float32	l,float32	r){

		if(l==r)
			return	true;
		float32	d=fabs(l-r);
		return	fabs(l-r)<FloatTolerance;
	}

	bool	Utils::Synchronous(uint64	l,uint64	r){

		return	abs((int32)(l-r))<TimeTolerance;
	}

	uint64	Utils::GetTimestamp(const	Atom	*iptr){

		uint64	high=iptr[1].atom;
		return	high<<32	|	iptr[2].atom;
	}

	void	Utils::SetTimestamp(Atom	*iptr,uint64	t){

		iptr[0]=Atom::Timestamp();
		iptr[1].atom=t>>32;
		iptr[2].atom=t	&	0x00000000FFFFFFFF;
	}

	void	Utils::SetTimestamp(Code	*object,uint16	index,uint64	t){

		object->code(index)=Atom::Timestamp();
		object->code(++index)=Atom(t>>32);
		object->code(++index)=Atom(t	&	0x00000000FFFFFFFF);
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

	int32	Utils::GetResilience(uint64	now,uint64	time_to_live,uint64	upr){

		if(time_to_live==0	||	upr==0)
			return	1;
		uint64	deadline=now+time_to_live;
		uint64	last_upr=(now-TimeReference)/upr;
		uint64	next_upr=(deadline-TimeReference)/upr;
		if((deadline-TimeReference)%upr>0)
			++next_upr;
		return	next_upr-last_upr;
	}

	int32	Utils::GetResilience(float32	resilience,float32	origin_upr,float32	destination_upr){

		if(origin_upr==0)
			return	1;
		if(destination_upr<=origin_upr)
			return	1;
		float32	r=origin_upr/destination_upr;
		float32	res=resilience*r;
		if(res<1)
			return	1;
		return	res;
	}
}