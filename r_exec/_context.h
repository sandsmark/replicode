//	_context.h
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

#ifndef	_context_h
#define	_context_h

#include	"../r_code/atom.h"
#include	"overlay.h"


using	namespace	r_code;

namespace	r_exec{

	//	Base class for evaluation contexts.
	//	Subclasses: IPGMContext and HLPContext.
	//	_Context	* wrapped in Context, the latter used by operators.
	class	dll_export	_Context{
	protected:
		Overlay	*const	overlay;	//	the overlay where the evaluation is performed; NULL when the context is dereferenced outside the original pgm or outside the value array.
		Atom			*code;		//	the object's code, or the code in value array, or the view's code when the context is dereferenced from Atom::VIEW.
		uint16			index;		//	in the code;

		typedef	enum{				//	indicates whether the context refers to:
			STEM=0,					//		- the pgm/hlp being reducing inputs;
			REFERENCE=1,			//		- a reference to another object;
			VIEW=2,					//		- a view;
			MKS=3,					//		- the mks or an object;
			VWS=4,					//		- the vws of an object;
			VALUE_ARRAY=5,			//		- code in the overlay's value array.
			BINDING_MAP=6,			//		- values of a imdl/icst.
			UNDEFINED=7
		}Data;
		Data	data;

		_Context(Atom	*code,uint16	index,Overlay	*overlay,Data	data):code(code),index(index),overlay(overlay),data(data){}
	public:
		virtual	_Context	*assign(const	_Context	*c)=0;

		virtual	bool	equal(const	_Context	*c)	const=0;

		virtual	Atom	&get_atom(uint16	i)	const=0;

		virtual	uint16	get_object_code_size()	const=0;

		virtual	uint16		getChildrenCount()			const=0;
		virtual	_Context	*_getChild(uint16	index)	const=0;
		
		virtual	_Context	*dereference()	const=0;

		void	commit()	const{	overlay->commit();		}
		void	rollback()	const{	overlay->rollback();	}
		void	patch_code(uint16	location,Atom	value)	const{	overlay->patch_code(location,value);	}
		void	unpatch_code(uint16	patch_index)	const{	overlay->unpatch_code(patch_index);	}
		uint16	get_last_patch_index()	const{	return	overlay->get_last_patch_index();	}

		uint16	setAtomicResult(Atom	a)		const;
		uint16	setTimestampResult(uint64	t)	const;
		uint16	setCompoundResultHead(Atom	a)	const;
		uint16	addCompoundResultPart(Atom	a)	const;

		void	trace()	const;
	};
}


#endif