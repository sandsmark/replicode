//	preprocessor.h
//
//	Author: Thor List, Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Thor List, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Thor List or Eric Nivel nor the
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

#ifndef	preprocessor_h
#define	preprocessor_h

#include	"segments.h"
#include	<istream>
#include	<sstream>
#include	<fstream>

using	namespace	r_code;

namespace	r_comp{

	class	RepliMacro;
	class	RepliCondition;
	class	RepliStruct{
	public:
		static	UNORDERED_MAP<std::string,RepliMacro	*>	RepliMacros;
		static	UNORDERED_MAP<std::string,int32>			Counters;
		static	std::list<RepliCondition	*>				Conditions;
		static	uint32										GlobalLine;

		enum Type {Root,Structure,Set,Atom,Directive,Condition,Development};
		Type type;
		std::string	cmd;
		std::string	tail;
		std::string	label;
		std::string	error;
		uint32	line;
		std::list<RepliStruct	*>	args;
		RepliStruct	*parent;

		RepliStruct(RepliStruct::Type type);
		~RepliStruct();

		uint32	getIndent(std::istream *stream);
		int32	parse(std::istream	*stream,uint32	&curIndent,uint32	&prevIndent,int32	paramExpect=0);
		bool	parseDirective(std::istream *stream,uint32	&curIndent, uint32	&prevIndent);
		int32	process();

		RepliStruct	*findAtom(const	std::string &name);
		RepliStruct	*loadReplicodeFile(const std::string &filename);

		RepliStruct	*clone()	const;
		std::string	print()	const;
		std::string	printError()	const;

		friend std::ostream& operator<<(std::ostream	&os,const	RepliStruct	&structure);
		friend std::ostream& operator<<(std::ostream	&os,RepliStruct	*structure);
	};

	class	RepliMacro{
	public:
		std::string	name;
		RepliStruct	*src;
		RepliStruct	*dest;
		std::string	error;

		RepliMacro(const	std::string	&name,RepliStruct	*src,RepliStruct	*dest);
		~RepliMacro();

		uint32	argCount();
		RepliStruct	*expandMacro(RepliStruct	*oldStruct);
	};

	class	RepliCondition{
	public:
		std::string	name;
		bool		reversed;

		RepliCondition(const	std::string	&name,bool	reversed);
		~RepliCondition();
		bool	reverse();
		bool	isActive(UNORDERED_MAP<std::string,RepliMacro*> &RepliMacros, UNORDERED_MAP<std::string,int32> &Counters);
	};

	class	dll_export	Preprocessor{
	private:
		typedef	enum{
			T_CLASS=0,
			T_SYS_CLASS=1,
			T_SET=2
		}ClassType;
		DefinitionSegment							*definition_segment;
		uint16										class_opcode;	//	shared with sys_classes
		UNORDERED_MAP<std::string,RepliStruct	*>	template_classes;
		void		instantiateClass(RepliStruct	*tpl_class,std::list<RepliStruct	*>	&tpl_args,std::string	&instantiated_class_name);
		bool		isSet(std::string	class_name);
		bool		isTemplateClass(RepliStruct	*s);
		void		getMember(std::vector<StructureMember>	&members,RepliStruct	*m,std::list<RepliStruct	*>	&tpl_args,bool	instantiate);
		void		getMembers(RepliStruct	*s,std::vector<StructureMember>	&members,std::list<RepliStruct	*>	&tpl_args,bool	instantiate);
		ReturnType	getReturnType(RepliStruct	*s);
		void		initialize();	//	init definition_segment
	public:
		RepliStruct	*root;

		Preprocessor();
		~Preprocessor();
		bool	process(DefinitionSegment	*definition_segment,	//	process will fill definition_segment
						std::istream		*stream,	//	if an ifstream, stream must be open
						std::ostringstream	*outstream,	//	output stream=input stream where macros are expanded
						std::string			*error);	//	set when function fails, e.g. returns false
	};

	//	For development only: hard codes the basic classes, does not expands any macros.
	//	The output stream is the same as the input stream.
	class	dll_export	HardCodedPreprocessor{
	private:
		void	initialize(DefinitionSegment	*definition_segment);
	public:
		HardCodedPreprocessor();
		~HardCodedPreprocessor();
		bool	process(DefinitionSegment	*definition_segment,	//	process will fill definition_segment
						std::istream		*stream,	//	if an ifstream, stream must be open
						std::ostringstream	*outstream,	//	output stream=input stream where macros are expanded
						std::string			*error);	//	set when function fails, e.g. returns false
	};
}


#endif
