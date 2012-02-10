//	hlp_context.h
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

#ifndef	hlp_context_h
#define	hlp_context_h

#include	"../r_code/object.h"

#include	"_context.h"
#include	"hlp_overlay.h"


namespace	r_exec{

	class	dll_export	HLPContext:
	public	_Context{
	public:
		HLPContext();
		HLPContext(Atom	*code,uint16	index,HLPOverlay	*const	overlay,Data	data=STEM);

		HLPContext	operator	*()	const;

		HLPContext	&operator	=(const	HLPContext	&c){

			code=c.code;
			index=c.index;
			return	*this;
		}

		Atom	&operator	[](uint16	i)	const{	return	code[index+i];	}

		bool	operator	==(const	HLPContext	&c)	const;
		bool	operator	!=(const	HLPContext	&c)	const;

		HLPContext	getChild(uint16	index)	const{

			return	HLPContext(code,this->index+index,(HLPOverlay	*)overlay);
		}

		bool	evaluate(uint16	&result_index)					const;	//	index is set to the index of the result, undefined in case of failure.
		bool	evaluate_no_dereference(uint16	&result_index)	const;

		//	__Context implementation.
		_Context	*assign(const	_Context	*c){

			HLPContext	*_c=new	HLPContext(*(HLPContext	*)c);
			return	_c;
		}

		bool	equal(const	_Context	*c)	const{	return	*this==*(HLPContext	*)c;	}

		Atom	&get_atom(uint16	i)	const{	return	this->operator	[](i);	}

		uint16	get_object_code_size()	const;

		uint16	getChildrenCount()		const{
			
			return	code[index].getAtomCount();
		}
		_Context	*_getChild(uint16	index)	const{

			HLPContext	*_c=new	HLPContext(getChild(index));
			return	_c;
		}
		
		_Context	*dereference()	const{

			HLPContext	*_c=new	HLPContext(**this);
			return	_c;
		}
	};
}


#endif