//	_context.cpp
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

#include	"_context.h"


namespace	r_exec{

	uint16	_Context::setAtomicResult(Atom	a)		const{	//	patch code with 32 bits data.
			
			overlay->patch_code(index,a);
			return	index;
		}

	uint16	_Context::setTimestampResult(uint64	t)	const{	//	patch code with a VALUE_PTR
			
		overlay->patch_code(index,Atom::ValuePointer(overlay->values.size()));
		overlay->values.as_std()->resize(overlay->values.size()+3);
		uint16	value_index=overlay->values.size()-3;
		Utils::SetTimestamp(&overlay->values[value_index],t);
		return	value_index;
	}

	uint16	_Context::setCompoundResultHead(Atom	a)	const{	//	patch code with a VALUE_PTR.
		
		uint16	value_index=overlay->values.size();
		overlay->patch_code(index,Atom::ValuePointer(value_index));
		addCompoundResultPart(a);
		return	value_index;
	}

	uint16	_Context::addCompoundResultPart(Atom	a)	const{	//	store result in the value array.
		
		overlay->values.push_back(a);
		return	overlay->values.size()-1;
	}

	void	_Context::trace()	const{

		std::cout<<"======== CONTEXT ========\n";
		switch(data){
		case	UNDEFINED:
			std::cout<<"undefined\n";
			return;
		case	MKS:
			std::cout<<"--> mks\n";
			return;
		case	VWS:
			std::cout<<"--> vws\n";
			return;
		}

		for(uint16	i=0;i<get_object_code_size();++i){

			if(index==i)
				std::cout<<">>";
			std::cout<<i<<"\t";
			code[i].trace();
			std::cout<<std::endl;
		}
	}
}