//	decompiler.h
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

#ifndef	decompiler_h
#define	decompiler_h

#include	<fstream>
#include	<sstream>

#include	"out_stream.h"
#include	"segments.h"


namespace	r_comp{

	class	dll_export	Decompiler{
	private:
		OutStream	*out_stream;
		uint16		indents;		//	in chars
		bool		closing_set;	//	set after writing the last element of a set: any element in an expression finding closeing_set will indent and set closing_set to false
		
		ImageObject	*current_object;

		r_comp::Metadata	*metadata;
		r_comp::Image		*image;

		uint64	time_offset;	//	0 means no offset.

		UNORDERED_MAP<uint16,std::string>	variable_names;				//	in the form vxxx where xxx is an integer representing the order of referencing of the variable/label in the code
		std::string	get_variable_name(uint16	index,bool	postfix);	//	associates iptr/vptr indexes to names; inserts them in out_stream if necessary; when postfix==true, a trailing ':' is added

		UNORDERED_MAP<uint16,std::string>	object_names;				//	in the form class_namexxx where xxx is an integer representing the order of appearence of the object in the image; N.B.: root:0 self:1 stdin:2 stdout:3
		UNORDERED_MAP<std::string,uint16>	object_indices;				//	inverted version of the object_names.
		std::string	get_object_name(uint16	index);			//	retrieves the name of an object

		void	write_indent(uint16	i);
		void	write_expression_head(uint16	read_index);						//	decodes the leading atom of an expression
		void	write_expression_tail(uint16	read_index,bool	vertical=false);	//	decodes the elements of an expression following the head
		void	write_expression(uint16	read_index);
		void	write_set(uint16	read_index);
		void	write_any(uint16	read_index,bool	&after_tail_wildcard);	//	decodes any element in an expression or a set
	public:
		Decompiler();
		~Decompiler();

		void	init(r_comp::Metadata	*metadata);
		uint32	decompile(r_comp::Image	*image,std::ostringstream	*stream,uint64	time_offset);	//	decompiles the whole image; returns the number of objects.
		uint32	decompile_references(r_comp::Image	*image);										//	initialize a reference table so that objects can be decompiled individually; returns the number of objects.
		void	decompile_object(uint16	object_index,std::ostringstream	*stream);					//	decompiles a single object; object_index is the position of the object in the vector returned by Image::getObject.
		void	decompile_object(const	std::string	object_name,std::ostringstream	*stream);		//	decompiles a single object given its name: use this function to follow references.
	};
}


#endif
