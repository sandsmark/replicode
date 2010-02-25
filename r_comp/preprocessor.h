#ifndef	preprocessor_h
#define	preprocessor_h

#include	"segments.h"
#include	<istream>
#include	<sstream>
#include	"string_utils.h"
#include	<fstream>

using	namespace	r_code;

namespace	r_comp{

	class	RepliMacro;
	class	RepliCondition;
	class	RepliStruct{
	public:
		// Global members for all instances
		static	UNORDERED_MAP<std::string,RepliMacro	*>	repliMacros;
		static	UNORDERED_MAP<std::string,int32>	counters;
		static	std::list<RepliCondition	*>	conditions;
		static	uint32	globalLine;

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
		bool	isActive(UNORDERED_MAP<std::string,RepliMacro*> &repliMacros, UNORDERED_MAP<std::string,int32> &counters);
	};

	class	dll_export	Preprocessor{
	private:
		void	initialize(DefinitionSegment	*definition_segment);
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
	friend	class	Preprocessor;
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
