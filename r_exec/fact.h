//	fact.h
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

#ifndef	fact_h
#define	fact_h


namespace	r_exec{

	class	Any{
	public:
		static	bool	Equal(const	Code	*lhs,const	Code	*rhs){
				
			if(lhs->code(0)!=rhs->code(0))
				return	false;
			if(lhs->code(0).asOpcode()==Opcodes::Ent	||	lhs->code(0).asOpcode()==Opcodes::Var)
				return	lhs==rhs;
			if(lhs->code_size()!=rhs->code_size())
				return	false;
			if(lhs->references_size()!=rhs->references_size())
				return	false;

			uint16	i;
			for(i=0;i<lhs->references_size();++i){

				if(lhs->get_reference(i)->code(0).asOpcode()==Opcodes::Var	||
					rhs->get_reference(i)->code(0).asOpcode()==Opcodes::Var)
					continue;
				if(!Any::Equal(lhs->get_reference(i),rhs->get_reference(i)))
					return	false;
			}
			for(i=0;i<lhs->code_size();){

				switch(lhs->code(i).getDescriptor()){
				case	Atom::NUMERICAL_VARIABLE:
					++i;
					break;
				case	Atom::STRUCTURAL_VARIABLE:
					i+=lhs->code(i-1).getAtomCount();
					break;
				default:
					if(lhs->code(i)!=rhs->code(i))
						return	false;
					++i;
					break;
				}
			}
			return	true;
		}
	};

	class	Fact{
	public:
		static	bool	Equal(const	Code	*lhs,const	Code	*rhs){	//	both operands are assumed to be of class fact or |fact.

			if(lhs->code(0)!=rhs->code(0))
				return	false;
			return	Any::Equal(lhs->get_reference(0),rhs->get_reference(0));	//	compare the pointed objects; ignore time.
		}
	};
}


#endif