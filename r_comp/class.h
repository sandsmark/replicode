//	class.h
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

#ifndef	class_h
#define	class_h

#include	"structure_member.h"


namespace	r_comp{

	class	Compiler;

	class	Class{
	private:
		bool	has_offset()	const;
	public:
		static	const	char	*Expression;
		static	const	char	*Type;

		Class(ReturnType	t=ANY);
		Class(Atom							atom,
			std::string						str_opcode,
			std::vector<StructureMember>	r,
			ReturnType						t=ANY);
		bool	is_pattern(Metadata	*metadata)	const;
		bool	get_member_index(Metadata	*metadata,std::string	&name,uint16	&index,Class	*&p)	const;
		std::string	get_member_name(uint32	index);	//	for decompilation
		ReturnType	get_member_type(const	uint16	index);
		Class		*get_member_class(Metadata	*metadata,const	std::string	&name);
		Atom							atom;
		std::string						str_opcode;			//	unused for anything but objects, markers and operators.
		std::vector<StructureMember>	things_to_read;
		ReturnType						type;				//	ANY for non-operators.
		StructureMember::Iteration		use_as;

		void	write(word32	*storage);
		void	read(word32		*storage);
		uint32	getSize();
	};
}


#endif